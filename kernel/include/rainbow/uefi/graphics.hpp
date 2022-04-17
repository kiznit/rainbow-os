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

#pragma once

#include "base.hpp"

namespace efi
{
    constexpr Guid GraphicsOutputProtocolGuid{0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

    struct PixelBitmask
    {
        uint32_t redMask;
        uint32_t greenMask;
        uint32_t blueMask;
        uint32_t reservedMask;
    };

    enum class PixelFormat
    {
        RedGreenBlueReserved8BitPerColor,
        BlueGreenRedReserved8BitPerColor,
        BitMask,
        BltOnly
    };

    struct GraphicsOutputModeInformation
    {
        uint32_t version;
        uint32_t horizontalResolution;
        uint32_t verticalResolution;
        PixelFormat pixelFormat;
        PixelBitmask pixelInformation;
        uint32_t pixelsPerScanLine;
    };

    struct GraphicsOutputBltPixel
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t reserved;
    };

    enum class GraphicsOutputBltOperation
    {
        ///
        /// Write data from the BltBuffer pixel (0, 0)
        /// directly to every pixel of the video display rectangle
        /// (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height).
        /// Only one pixel will be used from the BltBuffer. Delta is NOT used.
        ///
        BltVideoFill,

        ///
        /// Read data from the video display rectangle
        /// (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
        /// the BltBuffer rectangle (DestinationX, DestinationY )
        /// (DestinationX + Width, DestinationY + Height). If DestinationX or
        /// DestinationY is not zero then Delta must be set to the length in bytes
        /// of a row in the BltBuffer.
        ///
        BltVideoToBltBuffer,

        ///
        /// Write data from the BltBuffer rectangle
        /// (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
        /// video display rectangle (DestinationX, DestinationY)
        /// (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
        /// not zero then Delta must be set to the length in bytes of a row in the
        /// BltBuffer.
        ///
        BltBufferToVideo,

        ///
        /// Copy from the video display rectangle (SourceX, SourceY)
        /// (SourceX + Width, SourceY + Height) to the video display rectangle
        /// (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height).
        /// The BltBuffer and Delta are not used in this mode.
        ///
        BltVideoToVideo,

        GraphicsOutputBltOperationMax
    };

    struct GraphicsOutputProtocolMode
    {
        uint32_t maxMode;
        uint32_t mode;
        const GraphicsOutputModeInformation* info;
        uintn_t sizeOfInfo;
        PhysicalAddress framebufferBase;
        uintn_t frameBufferSize;
    };

    struct GraphicsOutputProtocol
    {
        Status(EFIAPI* QueryMode)(GraphicsOutputProtocol* self, uint32_t modeNumber, uintn_t* SizeOfInfo,
                                  GraphicsOutputModeInformation** info);
        Status(EFIAPI* SetMode)(GraphicsOutputProtocol* self, uint32_t modeNumber);
        Status(EFIAPI* Blt)(GraphicsOutputProtocol* self, const GraphicsOutputBltPixel* bltBuffer,
                            GraphicsOutputBltOperation nltOperation, uintn_t sourceX, uintn_t sourceY, uintn_t destinationX,
                            uintn_t destinationY, uintn_t width, uintn_t height, uintn_t delta);

        const GraphicsOutputProtocolMode* mode;
    };

} // namespace efi