/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <elf.h>
#include <rainbow/uefi.h>

#define R_AARCH64_RELATIVE  1027


extern const void* _DYNAMIC[];


extern "C" EFI_STATUS _relocate(const uintptr_t imageBase)
{
    const Elf64_Rela* table = nullptr; // Relocation table
    uint64_t tableSize  = 0; // Size of relocation table
    uint64_t entrySize  = 0; // Size of one relocation entry

    for (auto dyn = (const Elf64_Dyn*)((uintptr_t)_DYNAMIC + imageBase); dyn->d_tag != DT_NULL; ++dyn)
    {
        switch (dyn->d_tag)
        {
            case DT_RELA:
                table = (Elf64_Rela*)(dyn->d_un.d_ptr + imageBase);
                break;

            case DT_RELASZ:
                tableSize = dyn->d_un.d_val;
                break;

            case DT_RELAENT:
                entrySize = dyn->d_un.d_val;
                break;
        }
    }

    if (!table && entrySize == 0)
    {
        return EFI_SUCCESS;
    }

    if (!table || entrySize == 0)
    {
        return EFI_LOAD_ERROR;
    }

    for (auto rel = table; tableSize > 0; tableSize -= entrySize)
    {
        switch (ELF64_R_TYPE(rel->r_info))
        {
            case R_AARCH64_RELATIVE:
                *(uintptr_t*)(imageBase + rel->r_offset) = imageBase + rel->r_addend;
                break;
        }

        rel = (const Elf64_Rela*)((char*)rel + entrySize);
    }

    return EFI_SUCCESS;
}
