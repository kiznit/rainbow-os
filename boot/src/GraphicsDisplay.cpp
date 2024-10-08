/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "GraphicsDisplay.hpp"
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

GraphicsDisplay::GraphicsDisplay(efi::GraphicsOutputProtocol* gop, efi::EdidProtocol* edid) : m_gop(gop), m_edid(edid)
{
    (void)m_edid; // TODO: remove once we actually use m_edid

    InitSurfaces();
}

void GraphicsDisplay::InitSurfaces()
{
    const auto mode = m_gop->mode;
    const auto info = mode->info;
    const int width = info->horizontalResolution;
    const int height = info->verticalResolution;

    // Frontbuffer
    const auto pixelFormat = DeterminePixelFormat(*info);

    if (pixelFormat == mtl::PixelFormat::Unknown)
        m_frontbuffer.reset();
    else
        m_frontbuffer = mtl::make_shared<mtl::Surface>(width, height, info->pixelsPerScanLine * GetPixelSize(pixelFormat),
                                                       pixelFormat, (void*)(uintptr_t)mode->framebufferBase);

    // Backbuffer
    if (m_backbuffer && m_backbuffer->width == width && m_backbuffer->height == height)
        return;

    m_backbuffer = mtl::make_shared<mtl::Surface>(width, height, mtl::PixelFormat::X8R8G8B8);
}

int GraphicsDisplay::GetModeCount() const
{
    return m_gop->mode->maxMode;
}

void GraphicsDisplay::GetCurrentMode(mtl::GraphicsMode* mode) const
{
    const auto info = m_gop->mode->info;

    mode->width = info->horizontalResolution;
    mode->height = info->verticalResolution;
    mode->format = DeterminePixelFormat(*info);
}

bool GraphicsDisplay::GetMode(int index, mtl::GraphicsMode* mode) const
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
        return false;

    mode->width = info->horizontalResolution;
    mode->height = info->verticalResolution;
    mode->format = DeterminePixelFormat(*info);

    return true;
}

bool GraphicsDisplay::SetMode(int index)
{
    if (efi::Error(m_gop->SetMode(m_gop, index)))
        return false;

    InitSurfaces();

    return true;
}

mtl::shared_ptr<mtl::Surface> GraphicsDisplay::GetFrontbuffer()
{
    return m_frontbuffer;
}

mtl::shared_ptr<mtl::Surface> GraphicsDisplay::GetBackbuffer()
{
    return m_backbuffer;
}

void GraphicsDisplay::Blit(int x, int y, int width, int height)
{
    m_gop->Blt(m_gop, (efi::GraphicsOutputBltPixel*)m_backbuffer->pixels, efi::GraphicsOutputBltOperation::BltBufferToVideo, x, y,
               x, y, width, height, m_backbuffer->pitch);
}

// bool GraphicsDisplay::GetEdid(mtl::Edid* edid) const
// {
//     if (m_edid)
//         return edid->Initialize(m_edid->edid, m_edid->sizeOfEdid);
//     else
//         return false;
// }
