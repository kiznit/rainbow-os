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


#include "efidisplay.hpp"
#include "graphics/edid.hpp"


static PixelFormat DeterminePixelFormat(const EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info)
{
    // TODO
    // if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
    // {
    //     return PIXFMT_X8B8G8R8;
    // }
    //else
    if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
    {
        return PIXFMT_X8R8G8B8;
    }
    // TODO
    // else if (info->PixelFormat == PixelBitMask)
    // {
    //     return DeterminePixelFormat(
    //         info->PixelInformation.RedMask,
    //         info->PixelInformation.GreenMask,
    //         info->PixelInformation.BlueMask,
    //         info->PixelInformation.ReservedMask
    //     );
    // }
    else
    {
        return PIXFMT_UNKNOWN;
    }
}



EfiDisplay::EfiDisplay(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, EFI_EDID_ACTIVE_PROTOCOL* edid)
:   m_gop(gop),
    m_edid(edid)
{
}


int EfiDisplay::GetModeCount() const
{
    return m_gop->Mode->MaxMode;
}


void EfiDisplay::GetFramebuffer(Framebuffer* framebuffer) const
{
    const auto mode = m_gop->Mode;
    const auto info = mode->Info;
    const auto pixelFormat = DeterminePixelFormat(info);

    framebuffer->width = info->HorizontalResolution;
    framebuffer->height = info->VerticalResolution;
    framebuffer->format = pixelFormat;
    framebuffer->pitch = info->PixelsPerScanLine * GetPixelDepth(pixelFormat);
    framebuffer->pixels = (uintptr_t)mode->FrameBufferBase;
}


bool EfiDisplay::GetMode(int index, GraphicsMode* mode) const
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN size = sizeof(info);

    auto status = m_gop->QueryMode(m_gop, index, &size, &info);

    if (status == EFI_NOT_STARTED)
    {
        // Attempt to start GOP by calling SetMode()
        m_gop->SetMode(m_gop, m_gop->Mode->Mode);
        status = m_gop->QueryMode(m_gop, index, &size, &info);
    }

    if (EFI_ERROR(status))
    {
        return false;
    }

    mode->width = info->HorizontalResolution;
    mode->height = info->VerticalResolution;
    mode->format = DeterminePixelFormat(info);

    return true;
}


bool EfiDisplay::SetMode(int index)
{
    return EFI_ERROR(m_gop->SetMode(m_gop, index)) ? true : false;
}


bool EfiDisplay::GetEdid(Edid* edid) const
{
    if (m_edid)
    {
        return edid->Initialize(m_edid->Edid, m_edid->SizeOfEdid);
    }
    else
    {
        return false;
    }
}
