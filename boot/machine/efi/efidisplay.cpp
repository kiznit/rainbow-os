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
#include "boot.hpp"
#include "graphics/edid.hpp"
#include "graphics/graphicsconsole.hpp"
#include "graphics/pixels.hpp"
#include "graphics/surface.hpp"


static EFI_GUID g_efiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
static EFI_GUID g_efiEdidActiveProtocolGuid = EFI_EDID_ACTIVE_PROTOCOL_GUID;
static EFI_GUID g_efiEdidDiscoveredProtocolGuid = EFI_EDID_DISCOVERED_PROTOCOL_GUID;
static EFI_GUID g_efiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

static Surface g_frameBuffer;
static GraphicsConsole g_graphicsConsole;

extern EFI_BOOT_SERVICES* BS;


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


void EfiDisplay::GetCurrentMode(GraphicsMode* mode) const
{
    mode->width = m_gop->Mode->Info->HorizontalResolution;
    mode->height = m_gop->Mode->Info->VerticalResolution;
    mode->format = DeterminePixelFormat(m_gop->Mode->Info);
}


bool EfiDisplay::GetMode(int index, GraphicsMode* mode) const
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN size = sizeof(info);

    auto status = m_gop->QueryMode(m_gop, index, &size, &info);
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


EFI_STATUS InitDisplays()
{
    UINTN size = 0;
    EFI_HANDLE* handles = nullptr;
    EFI_STATUS status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = BS->LocateHandle(ByProtocol, &g_efiGraphicsOutputProtocolGuid, nullptr, &size, handles)) == EFI_BUFFER_TOO_SMALL)
    {
        handles = (EFI_HANDLE*)realloc(handles, size);
        if (!handles)
        {
            return EFI_OUT_OF_RESOURCES;
        }
    }

    if (EFI_ERROR(status))
    {
        return status;
    }

    const int count = size / sizeof(EFI_HANDLE);

    for (int i = 0; i != count; ++i)
    {
        EFI_DEVICE_PATH_PROTOCOL* dpp = nullptr;
        BS->HandleProtocol(handles[i], &g_efiDevicePathProtocolGuid, (void**)&dpp);
        // If dpp is NULL, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp) continue;

        EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
        BS->HandleProtocol(handles[i], &g_efiGraphicsOutputProtocolGuid, (void**)&gop);
        // gop is not expected to be null, but let's play safe.
        if (!gop) continue;

        EFI_EDID_ACTIVE_PROTOCOL* edid = nullptr;
        if (EFI_ERROR(BS->HandleProtocol(handles[i], &g_efiEdidActiveProtocolGuid, (void**)&edid)) || !edid)
        {
            BS->HandleProtocol(handles[i], &g_efiEdidDiscoveredProtocolGuid, (void**)&edid);
        }

        EfiDisplay display(gop, edid);

        SetBestMode(display);

        if (g_bootInfo.framebufferCount < ARRAY_LENGTH(BootInfo::framebuffers))
        {
            Framebuffer* fb = &g_bootInfo.framebuffers[g_bootInfo.framebufferCount];
            auto info = gop->Mode->Info;
            auto pixelFormat = DeterminePixelFormat(info);

            fb->width = info->HorizontalResolution;
            fb->height = info->VerticalResolution;
            fb->pitch = info->PixelsPerScanLine * GetPixelDepth(pixelFormat);
            fb->format = pixelFormat;
            fb->pixels = (uintptr_t)gop->Mode->FrameBufferBase;

            g_bootInfo.framebufferCount += 1;
        }
    }

    free(handles);


    // Initialize the graphics console
    if (g_bootInfo.framebufferCount > 0)
    {
        Framebuffer* fb = g_bootInfo.framebuffers;

        g_frameBuffer.width = fb->width;
        g_frameBuffer.height = fb->height;
        g_frameBuffer.pitch = fb->pitch;
        g_frameBuffer.pixels = (void*)fb->pixels;
        g_frameBuffer.format = fb->format;

        g_graphicsConsole.Initialize(&g_frameBuffer);
        g_graphicsConsole.Clear();

        g_console = &g_graphicsConsole;
    }

    return EFI_SUCCESS;
}
