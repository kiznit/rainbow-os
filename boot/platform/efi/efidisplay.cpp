/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "efidisplay.hpp"
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>



bool EfiDisplay::Initialize(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop)
{
    m_gop = gop;
    return true;
}



int EfiDisplay::GetModeCount() const
{
    return m_gop->Mode->MaxMode;
}



bool EfiDisplay::GetMode(int index, DisplayMode* displayInfo) const
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN size = sizeof(info);
    if (EFI_ERROR(m_gop->QueryMode(m_gop, index, &size, &info)))
    {
        return false;
    }

    displayInfo->width = info->HorizontalResolution;
    displayInfo->height = info->VerticalResolution;

    if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
    {
        displayInfo->format = PIXFMT_X8B8G8R8;
        displayInfo->pitch = info->PixelsPerScanLine * 4;
    }
    else if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
    {
        displayInfo->format = PIXFMT_X8R8G8B8;
        displayInfo->pitch = info->PixelsPerScanLine * 4;
    }
    else if (info->PixelFormat == PixelBitMask)
    {
        displayInfo->format = DeterminePixelFormat(
            info->PixelInformation.RedMask,
            info->PixelInformation.GreenMask,
            info->PixelInformation.BlueMask,
            info->PixelInformation.ReservedMask
        );
        displayInfo->pitch = info->PixelsPerScanLine * GetPixelDepth(displayInfo->format);
    }
    else
    {
        displayInfo->format = PIXFMT_UNKNOWN;
        displayInfo->pitch = 0;
    }

    return true;
}



bool EfiDisplay::SetMode(int mode) const
{
    (void)mode;
    return false;
}
