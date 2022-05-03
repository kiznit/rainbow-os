/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "EfiDisplay.hpp"
#include <metal/log.hpp>

static mtl::PixelFormat DeterminePixelFormat(const efi::GraphicsOutputModeInformation& info)
{
    switch (info.pixelFormat)
    {
    case efi::PixelFormat::RedGreenBlueReserved8BitPerColor:
        return mtl::PixelFormat::X8B8G8R8;

    case efi::PixelFormat::BlueGreenRedReserved8BitPerColor:
        return mtl::PixelFormat::X8R8G8B8;

    case efi::PixelFormat::BitMask:
        return mtl::DeterminePixelFormat(info.pixelInformation.redMask, info.pixelInformation.greenMask,
                                         info.pixelInformation.blueMask, info.pixelInformation.reservedMask);

    case efi::PixelFormat::BltOnly:
    default:
        return mtl::PixelFormat::Unknown;
    }
}

EfiDisplay::EfiDisplay(efi::GraphicsOutputProtocol* gop, efi::EdidProtocol* edid) : m_gop(gop), m_edid(edid)
{
    InitBackbuffer();
}

EfiDisplay::EfiDisplay(EfiDisplay&& other)
    : m_gop(std::exchange(other.m_gop, nullptr)), m_edid(std::exchange(other.m_edid, nullptr)),
      m_backbuffer(std::move(other.m_backbuffer))
{}

void EfiDisplay::InitBackbuffer()
{
    const auto info = m_gop->mode->info;
    const int width = info->horizontalResolution;
    const int height = info->verticalResolution;

    if (m_backbuffer && m_backbuffer->width == width && m_backbuffer->height == height)
    {
        return;
    }

    if (m_backbuffer)
    {
        delete[](uint32_t*) m_backbuffer->pixels;
    }
    else
    {
        m_backbuffer.reset(new mtl::Surface());
    }

    m_backbuffer->width = width;
    m_backbuffer->height = height;
    m_backbuffer->pitch = width * 4;
    m_backbuffer->format = mtl::PixelFormat::X8R8G8B8;
    m_backbuffer->pixels = new uint32_t[width * height];
}

int EfiDisplay::GetModeCount() const
{
    return m_gop->mode->maxMode;
}

void EfiDisplay::GetCurrentMode(mtl::GraphicsMode* mode) const
{
    const auto info = m_gop->mode->info;

    mode->width = info->horizontalResolution;
    mode->height = info->verticalResolution;
    mode->format = DeterminePixelFormat(*info);
}

bool EfiDisplay::GetMode(int index, mtl::GraphicsMode* mode) const
{
    efi::GraphicsOutputModeInformation* info;
    efi::uintn_t size = sizeof(info);

    auto status = m_gop->QueryMode(m_gop, index, &size, &info);

    if (status == efi::Status::NotStarted)
    {
        // Attempt to start GOP by calling SetMode()
        m_gop->SetMode(m_gop, m_gop->mode->mode);
        status = m_gop->QueryMode(m_gop, index, &size, &info);
    }

    if (efi::Error(status))
    {
        return false;
    }

    mode->width = info->horizontalResolution;
    mode->height = info->verticalResolution;
    mode->format = DeterminePixelFormat(*info);

    return true;
}

bool EfiDisplay::SetMode(int index)
{
    if (efi::Error(m_gop->SetMode(m_gop, index)))
    {
        return false;
    }

    InitBackbuffer();

    return true;
}

mtl::Surface* EfiDisplay::GetBackbuffer()
{
    return m_backbuffer.get();
}

void EfiDisplay::Blit(int x, int y, int width, int height)
{
    m_gop->Blt(m_gop, (efi::GraphicsOutputBltPixel*)m_backbuffer->pixels, efi::GraphicsOutputBltOperation::BltBufferToVideo, x, y,
               x, y, width, height, m_backbuffer->pitch);
}

// bool EfiDisplay::GetFramebuffer(Framebuffer* framebuffer)
// {
//     const auto mode = m_gop->Mode;
//     const auto info = mode->Info;
//     const auto pixelFormat = DeterminePixelFormat(info);

//     framebuffer->width = info->HorizontalResolution;
//     framebuffer->height = info->VerticalResolution;
//     framebuffer->format = pixelFormat;
//     framebuffer->pitch = info->PixelsPerScanLine * GetPixelDepth(pixelFormat);
//     framebuffer->pixels = (uintptr_t)mode->FrameBufferBase;

//     return pixelFormat != PixelFormat::Unknown;
// }

// bool EfiDisplay::GetEdid(mtl::Edid* edid) const
// {
//     if (m_edid)
//     {
//         return edid->Initialize(m_edid->edid, m_edid->sizeOfEdid);
//     }
//     else
//     {
//         return false;
//     }
// }

mtl::SimpleDisplay* EfiDisplay::ToSimpleDisplay()
{
    const auto mode = m_gop->mode;
    const auto info = mode->info;
    const auto pixelFormat = DeterminePixelFormat(*info);

    if (pixelFormat == mtl::PixelFormat::Unknown)
        return nullptr;

    auto frontbuffer = std::unique_ptr<mtl::Surface>(
        new mtl::Surface{.width = static_cast<int>(info->horizontalResolution),
                         .height = static_cast<int>(info->verticalResolution),
                         .pitch = static_cast<int>(info->pixelsPerScanLine * GetPixelDepth(pixelFormat)),
                         .format = pixelFormat,
                         .pixels = (void*)(uintptr_t)mode->framebufferBase});

    return new mtl::SimpleDisplay(frontbuffer.release(), m_backbuffer.release());
}

std::vector<EfiDisplay> InitializeDisplays(efi::BootServices* bootServices)
{
    std::vector<EfiDisplay> displays;

    efi::uintn_t size{0};
    std::vector<efi::Handle> handles;
    efi::Status status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = bootServices->LocateHandle(efi::LocateSearchType::ByProtocol, &efi::GraphicsOutputProtocolGuid, nullptr, &size,
                                                handles.data())) == efi::Status::BufferTooSmall)
    {
        handles.resize(size / sizeof(efi::Handle));
    }

    if (efi::Error(status))
    {
        // Likely efi::NotFound, but any error should be handled as "no display available"
        MTL_LOG(Warning) << "Not UEFI displays found: " << mtl::hex(status);
        return displays;
    }

    for (auto handle : handles)
    {
        efi::DevicePathProtocol* dpp = nullptr;
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::DevicePathProtocolGuid, (void**)&dpp)))
            continue;

        // If dpp is null, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp)
            continue;

        efi::GraphicsOutputProtocol* gop{nullptr};
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::GraphicsOutputProtocolGuid, (void**)&gop)))
            continue;
        // gop is not expected to be null, but let's play safe.
        if (!gop)
            continue;

        efi::EdidProtocol* edid{nullptr};
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::EdidActiveProtocolGuid, (void**)&edid)) || (!edid))
        {
            if (efi::Error(bootServices->HandleProtocol(handle, &efi::EdidDiscoveredProtocolGuid, (void**)&edid)))
                edid = nullptr;
        }

        const auto& mode = *gop->mode->info;
        MTL_LOG(Info) << "Display: " << mode.horizontalResolution << " x " << mode.verticalResolution
                      << ", edid size: " << (edid ? edid->sizeOfEdid : 0) << " bytes";

        displays.emplace_back(gop, edid);
    }

    return displays;
}
