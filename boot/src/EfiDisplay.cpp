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
#include <metal/graphics/Edid.hpp>
#include <rainbow/boot.hpp>
#include <utility>

static mtl::PixelFormat DeterminePixelFormat(const efi::GraphicsOutputModeInformation& info)
{
    switch (info.pixelFormat)
    {
    case efi::PixelRedGreenBlueReserved8BitPerColor:
        return mtl::PixelFormat::X8B8G8R8;

    case efi::PixelBlueGreenRedReserved8BitPerColor:
        return mtl::PixelFormat::X8R8G8B8;

    case efi::PixelBitMask:
        return mtl::DeterminePixelFormat(
            info.pixelInformation.redMask, info.pixelInformation.greenMask,
            info.pixelInformation.blueMask, info.pixelInformation.reservedMask);

    case efi::PixelBltOnly:
    default:
        return mtl::PixelFormat::Unknown;
    }
}

EfiDisplay::EfiDisplay(efi::GraphicsOutputProtocol* gop, efi::EdidProtocol* edid)
    : m_gop(gop), m_edid(edid)
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

    // TODO: the following code is a duplicate of VbeDisplay::InitBackBuffer()
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

    if (status == efi::NotStarted)
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
    m_gop->Blt(m_gop, (efi::GraphicsOutputBltPixel*)m_backbuffer->pixels, efi::BltBufferToVideo, x,
               y, x, y, width, height, m_backbuffer->pitch);
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

// SimpleDisplay* EfiDisplay::ToSimpleDisplay()
// {
//     std::unique_ptr<Surface> frontbuffer;

//     Framebuffer framebuffer;
//     if (GetFramebuffer(&framebuffer) && framebuffer.format != PixelFormat::Unknown)
//     {
//         frontbuffer.reset(new Surface{framebuffer.width, framebuffer.height, framebuffer.pitch,
//                                       framebuffer.format, (void*)framebuffer.pixels});
//     }

//     auto display = new SimpleDisplay();
//     display->Initialize(frontbuffer.release(), m_backbuffer.release());
//     return display;
// }
