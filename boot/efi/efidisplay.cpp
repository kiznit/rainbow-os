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


#include <rainbow/uefi.h>
#include <Protocol/EdidActive.h>
#include <Protocol/GraphicsOutput.h>
#include "boot.hpp"
#include "graphics/graphicsconsole.hpp"
#include "graphics/pixels.hpp"
#include "graphics/surface.hpp"


static EFI_GUID g_efiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
static EFI_GUID g_efiEdidActiveProtocolGuid = EFI_EDID_ACTIVE_PROTOCOL_GUID;
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



static void InitDisplay(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, EFI_EDID_ACTIVE_PROTOCOL* edid)
{
    // Start with the current mode as the "best"
    auto bestModeIndex = gop->Mode->Mode;
    auto bestModeInfo = *gop->Mode->Info;

    // TODO: use edid information if available to determine optimal resolution
    (void)edid;
    const auto maxWidth = bestModeInfo.HorizontalResolution;
    const auto maxHeight = bestModeInfo.VerticalResolution;

    for (unsigned i = 0; i != gop->Mode->MaxMode; ++i)
    {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
        UINTN size = sizeof(info);

        auto status = gop->QueryMode(gop, i, &size, &info);
        if (EFI_ERROR(status)) continue;

        const PixelFormat pixfmt = DeterminePixelFormat(info);
        if (pixfmt == PIXFMT_UNKNOWN) continue;

        // We want to keep the highest resolution possible, but not exceed the "ideal" one.
        if (info->HorizontalResolution > maxWidth || info->VerticalResolution > maxHeight) continue;

        if (info->HorizontalResolution > bestModeInfo.HorizontalResolution ||
            info->VerticalResolution > bestModeInfo.VerticalResolution)
        {
            bestModeIndex = i;
            bestModeInfo = *info;
        }
    }

    if (gop->Mode->Mode != bestModeIndex)
    {
        gop->SetMode(gop, bestModeIndex);
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

        EFI_EDID_ACTIVE_PROTOCOL* edid = nullptr;
        BS->HandleProtocol(handles[i], &g_efiEdidActiveProtocolGuid, (void**)&edid);

        InitDisplay(gop, edid);

        if (g_console != &g_graphicsConsole)
        {
            auto info = gop->Mode->Info;
            auto pixfmt = DeterminePixelFormat(info);
            g_frameBuffer.width = info->HorizontalResolution;
            g_frameBuffer.height = info->VerticalResolution;
            g_frameBuffer.pitch = info->PixelsPerScanLine * GetPixelDepth(pixfmt);
            g_frameBuffer.pixels = (void*)(uintptr_t)gop->Mode->FrameBufferBase;
            g_frameBuffer.format = pixfmt;
            g_graphicsConsole.Initialize(&g_frameBuffer);
            g_graphicsConsole.Clear();
            g_console = &g_graphicsConsole;
        }

        // Log("    Display #%d:\n", i);
        // Log("        GOP        : %p\n", gop);
        // Log("        EDID       : %p\n", edid);
        // Log("        Resolution : %d x %d\n", gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution);
        // Log("        Framebuffer: %X\n", gop->Mode->FrameBufferBase);
        // Log("        Size       : %X\n", gop->Mode->FrameBufferSize);
    }

    free(handles);

    return EFI_SUCCESS;
}
