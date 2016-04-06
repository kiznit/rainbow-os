/*
    Copyright (c) 2015, Thierry Tremblay
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

#ifndef _RAINBOW_RAINBOW_BOOT_H
#define _RAINBOW_RAINBOW_BOOT_H

#include <rainbow/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define RAINBOW_BOOT_MAGIC (((uint8_t)'R') | ((uint8_t)'B' << 8) | ((uint8_t)'O' << 16) | ((uint8_t)'W' << 24))
#define RAINBOW_BOOT_VERSION 1

typedef enum
{
    Firmware_Unknown,           // Unknown firmware
    Firmware_BIOS,              // BIOS
    Firmware_EFI,               // EFI / UEFI

} Firmware;



typedef enum
{
    FrameBufferType_None,       // No / unknown frame buffer format
    FrameBufferType_VGAText,    // Frame buffer is VGA text (2 bytes per character)
    FrameBufferType_RGB,        // Frame buffer is RGB

} FrameBufferType;



typedef enum
{
    PixelFormat_Invalid,        // Invalid
    PixelFormat_X8R8G8B8,       // 32 bits
    PixelFormat_X8B8G8R8,       // 32 bits
    PixelFormat_R8G8B8,         // 24 bits
    PixelFormat_R5G6B5,         // 16 bits
    PixelFormat_X1R5G5B5,       // 15 bits

} PixelFormat;



typedef struct
{
    uint32_t    type;           // Frame buffer type (FrameBufferType)
    uint32_t    pixelFormat;    // Pixel format (PixelFormat)
    uint32_t    width;          // In pixels (or characters)
    uint32_t    height;         // In pixels (or characters)
    uint32_t    pitch;          // In bytes
    uint64_t    address;        // Start of frame buffer in memory

} FrameBufferInfo;



typedef struct
{
    uint32_t            magic;              // Signature (RAINBOW_BOOT_MAGIC)
    uint32_t            version;            // Version (RAINBOW_BOOT_VERSION)
    uint32_t            reserved;           // Reserved for future use (Currently set to 0)
    uint32_t            checksum;           // The above fields plus this one must equal 0 mod 2^32

    uint32_t            firmware;           // Host firmware
    uint32_t            frameBufferCount;   // How many frame buffers are available (0-4)
    FrameBufferInfo     framebuffers[4];    // Active video frame buffers

} BootInfo;


#ifdef __cplusplus
}
#endif

#endif
