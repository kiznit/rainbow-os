/*
    Copyright (c) 2023, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Acpi.hpp"
#include "AcpiImpl.hpp"
#include "interrupt.hpp"
#include "lai.hpp"
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>
#include <metal/arch.hpp>
#include <metal/log.hpp>

using namespace std::literals;

extern const Acpi* g_lai_acpi;

static ErrorCode MapLaiErrorCode(lai_api_error_t error)
{
    switch (error)
    {
    case LAI_ERROR_NONE:
        return ErrorCode::NoError;
    case LAI_ERROR_OUT_OF_MEMORY:
        return ErrorCode::OutOfMemory;
    case LAI_ERROR_TYPE_MISMATCH:
        return ErrorCode::InvalidArguments;
    case LAI_ERROR_NO_SUCH_NODE:
        return ErrorCode::InvalidArguments;
    case LAI_ERROR_OUT_OF_BOUNDS:
        return ErrorCode::InvalidArguments;
    case LAI_ERROR_EXECUTION_FAILURE:
        return ErrorCode::Unexpected;
    case LAI_ERROR_ILLEGAL_ARGUMENTS:
        return ErrorCode::InvalidArguments;
    /* Evaluating external inputs (e.g., nodes of the ACPI namespace) returned an unexpected result.
     * Unlike LAI_ERROR_EXECUTION_FAILURE, this error does not indicate that
     * execution of AML failed; instead, the resulting object fails to satisfy some
     * expectation (e.g., it is of the wrong type, has an unexpected size, or consists of
     * unexpected contents) */
    case LAI_ERROR_UNEXPECTED_RESULT:
        return ErrorCode::Unexpected;
    // Error given when end of iterator is reached, nothing to worry about
    case LAI_ERROR_END_REACHED:
        return ErrorCode::NoError;
    case LAI_ERROR_UNSUPPORTED:
        return ErrorCode::Unsupported;
    }
}

static void AcpiLogTable(const AcpiTable& table, mtl::PhysicalAddress address)
{
    if (table.VerifyChecksum())
        MTL_LOG(Info) << "[ACPI] Table " << table.GetSignature() << " found at " << mtl::hex(address) << ", Checksum OK";
    else
        MTL_LOG(Info) << "[ACPI] Table " << table.GetSignature() << " found at " << mtl::hex(address) << ", Checksum FAILED";
}

template <typename T>
static void AcpiLogTables(const T& rootTable)
{
    for (auto address : rootTable)
    {
        const auto table = AcpiMapTable(address);
        AcpiLogTable(*table, address);

        if (table->GetSignature() == "FACP"sv)
        {
            const auto fadt = static_cast<const AcpiFadt*>(table);
            const PhysicalAddress dsdtAddress = AcpiTableContains(fadt, X_DSDT) ? fadt->X_DSDT : fadt->DSDT;
            const auto dsdt = AcpiMapTable(dsdtAddress);
            AcpiLogTable(*dsdt, dsdtAddress);
        }
    }
}

std::expected<void, ErrorCode> Acpi::Initialize(const AcpiRsdp& rsdp)
{
    if (m_initialized)
    {
        MTL_LOG(Error) << "[ACPI] ACPI is already initialized";
        return {};
    }

    if (rsdp.revision >= 2 && static_cast<const AcpiRsdpExtended&>(rsdp).xsdtAddress)
    {
        m_xsdt = AcpiMapTable<AcpiXsdt>(static_cast<const AcpiRsdpExtended&>(rsdp).xsdtAddress);
        MTL_LOG(Info) << "[ACPI] Using ACPI XSDT with revision " << rsdp.revision;
    }
    else if (rsdp.rsdtAddress)
    {
        m_rsdt = AcpiMapTable<AcpiRsdt>(rsdp.rsdtAddress);
        MTL_LOG(Info) << "[ACPI] Using ACPI RSDT with revision " << rsdp.revision;
    }
    else
    {
        MTL_LOG(Fatal) << "[ACPI] No ACPI RSDP table found";
        return std::unexpected(ErrorCode::Unsupported);
    }

    if (m_xsdt)
        AcpiLogTables(*m_xsdt);
    else
        AcpiLogTables(*m_rsdt);

    g_lai_acpi = this;

    m_fadt = FindTable<AcpiFadt>("FACP");
    if (!m_fadt)
    {
        MTL_LOG(Fatal) << "[ACPI] FADT not found";
        return std::unexpected(ErrorCode::Unexpected);
    }

    lai_set_acpi_revision(rsdp.revision);
    lai_create_namespace();

    m_initialized = true;
    return {};
}

std::expected<void, ErrorCode> Acpi::Enable(InterruptModel model)
{
    if (!m_initialized)
    {
        MTL_LOG(Error) << "[ACPI] ACPI has not been initialized";
        return {};
    }

    if (m_enabled)
    {
        MTL_LOG(Warning) << "[ACPI] ACPI is already initialized";
        return {};
    }

    // Register the ACPI interrupt handler
    if (!IsHardwareReduced())
    {
        // TODO: OSPM is required to treat the ACPI SCI interrupt as a sharable, level, active low interrupt.
        MTL_LOG(Info) << "[ACPI] SCI interrupt: " << m_fadt->SCI_INT;
        InterruptRegister(m_fadt->SCI_INT, *this);
    }

    const int result = lai_enable_acpi(static_cast<uint32_t>(model));
    if (result != 0)
    {
        MTL_LOG(Warning) << "[ACPI] Failed to enable ACPI: " << result;
        return std::unexpected(ErrorCode::Unexpected);
    }

    m_enabled = true;
    return {};
}

template <typename T>
static const AcpiTable* FindTableImpl(const T& rootTable, std::string_view signature, int index)
{
    int count = 0;
    for (auto address : rootTable)
    {
        const auto table = AcpiMapTable(address);
        if (table->GetSignature() == signature)
        {
            if (table->VerifyChecksum())
            {
                if (index == count)
                    return table;

                ++count;
            }
            else
                MTL_LOG(Warning) << "[ACPI] " << signature << " checksum is invalid in FindTable()";
        }
    }

    return nullptr;
}

const AcpiTable* Acpi::FindTable(std::string_view signature, int index) const
{
    if (signature == "DSDT"sv)
    {
        const auto fadt = FindTable<AcpiFadt>("FACP");
        if (!fadt)
            return nullptr;

        const PhysicalAddress dsdtAddress = AcpiTableContains(fadt, X_DSDT) ? fadt->X_DSDT : fadt->DSDT;
        return AcpiMapTable(dsdtAddress);
    }

    if (m_xsdt)
        return FindTableImpl(*m_xsdt, signature, index);
    else if (m_rsdt)
        return FindTableImpl(*m_rsdt, signature, index);
    else
        return nullptr;
}

bool Acpi::HandleInterrupt(InterruptContext*)
{
    // TODO: locking

    const auto event = lai_get_sci_event();
    MTL_LOG(Warning) << "[ACPI] Unhandled SCI event: " << mtl::hex(event);

    // TODO: handle the event appropriately

    return true;
}

bool Acpi::IsHardwareReduced() const
{
    return lai_current_instance()->is_hw_reduced;
}

std::expected<void, ErrorCode> Acpi::ResetSystem()
{
    auto result = lai_acpi_reset();
    auto errorCode = MapLaiErrorCode(result);
    if (errorCode != ErrorCode::NoError)
        return std::unexpected(errorCode);

    return {};
}

std::expected<void, ErrorCode> Acpi::SleepSystem(SleepState state)
{
    auto result = lai_enter_sleep(static_cast<uint8_t>(state));
    auto errorCode = MapLaiErrorCode(result);
    if (errorCode != ErrorCode::NoError)
        return std::unexpected(errorCode);

    return {};
}

/*
static void AcpiEnumerateNamespace(const LaiNsNode& node, int depth = 0)
{
    if (node.type == LAI_NAMESPACE_DEVICE)
    {
        const auto name = node.GetName();
        MTL_LOG(Info) << "[ACPI] Found device at depth " << depth << ":" << name;
    }
    else if (node.type == LAI_NAMESPACE_PROCESSOR)
    {
        const auto name = node.GetName();
        MTL_LOG(Info) << "[ACPI] Found processor at depth " << depth << ":" << name;
    }

    for (const auto& child : node)
    {
        AcpiEnumerateNamespace(child, depth + 1);
    }
}

void AcpiEnumerateNamespace()
{
    MTL_LOG(Info) << "[ACPI] AcpiEnumerateNamespace()";

    auto root = static_cast<LaiNsNode*>(lai_ns_get_root());
    AcpiEnumerateNamespace(*root);
}
*/
