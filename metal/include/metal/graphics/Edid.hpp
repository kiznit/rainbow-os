/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "VideoMode.hpp"
#include <cstddef>
#include <cstdint>
#include <metal/string.hpp>
#include <metal/vector.hpp>

/*
    August 1994, DDC standard version 1 – EDID v1.0 structure.
    April 1996, EDID standard version 2 – EDID v1.1 structure.
    1997, EDID standard version 3 – EDID structures v1.2 and v2.0
    February 2000, E-EDID Standard Release A, v1.0 – EDID structure v1.3,
                   EDID structure v2.0 deprecated
    September 2006 – E-EDID Standard Release A, v2.0 – EDID structure v1.4
*/

namespace mtl
{
    enum EdidFeatures : uint8_t
    {
        PreferredTimingMode = 0x02,
        sRGB = 0x04,
    };

    // EDID Data Block version 1.x
    struct EdidDataBlock
    {
        // Header
        uint8_t header[8]; // 00 FF FF FF FF FF FF 00

        // Vendor / product ID
        uint8_t manufacturerID[2];
        uint8_t productCodeID[2];
        uint8_t serialNumberID[4];
        uint8_t weekOfManufacture;
        uint8_t yearOfManufactury;

        // EDID structure version / revision
        uint8_t version;
        uint8_t revision;

        // Basic Display Parameters and Features
        uint8_t videoInputDefinition;
        uint8_t maxHorizontalImageSize; // in cm
        uint8_t maxVerticalImageSize;   // in cm
        uint8_t gamma;                  // (gamma * 100) - 100, range [1..3.54]
        EdidFeatures features;

        // Chromaticity, 10-bit CIE xy coordinates for red, green, blue, and white. [0–1023/1024].
        uint8_t redGreenLowBits;
        uint8_t blueWhiteLowBits;
        uint8_t redHighBitsX;
        uint8_t redHighBitsY;
        uint8_t greenHighBitsX;
        uint8_t greenHighBitsY;
        uint8_t blueHighBitsX;
        uint8_t blueHighBitsY;
        uint8_t whiteHighBitsX;
        uint8_t whiteHighBitsY;

        // Timings
        uint8_t establishedTimings[3];
        uint8_t standardTimings[16];
        uint8_t detailedTimings[4][18]; // NOTE: EDID 1 and 2 allowed this space to be used for
                                        // Monitor Descriptors

        // Trailer
        uint8_t extensionCount;
        uint8_t checksum;

        // Is the Edid data valid?
        bool Valid() const;

        mtl::string ManufacturerId() const
        {
            const uint16_t manufacturer = (manufacturerID[0] << 8) | manufacturerID[1];

            const char id[3]{static_cast<char>(((manufacturer >> 10) & 0x1F) + 'A' - 1),
                             static_cast<char>(((manufacturer >> 5) & 0x1F) + 'A' - 1),
                             static_cast<char>(((manufacturer >> 10) & 0x1F) + 'A' - 1)};

            return mtl::string(id, 3);
        }

        // Return the serial number
        constexpr unsigned int SerialNumber() const
        {
            return (serialNumberID[0] << 24) | (serialNumberID[1] << 16) | (serialNumberID[2] << 8) | serialNumberID[3];
        }

        // Returns Gamma (scaled by 100)
        // TODO: if this is 0xFF, gamma is not defined here... find out where it is and return it
        // (do not assume it is 2.2)
        constexpr int Gamma() const { return gamma == 0xFF ? 220 : gamma + 100; }

        // CIE xy coordinates [0..1023]
        constexpr int RedX() const { return (redHighBitsX << 2) | ((redGreenLowBits >> 6) & 3); }
        constexpr int RedY() const { return (redHighBitsY << 2) | ((redGreenLowBits >> 4) & 3); }
        constexpr int GreenX() const { return (greenHighBitsX << 2) | ((redGreenLowBits >> 2) & 3); }
        constexpr int GreenY() const { return (greenHighBitsY << 2) | ((redGreenLowBits >> 0) & 3); }
        constexpr int BlueX() const { return (blueHighBitsX << 2) | ((blueWhiteLowBits >> 6) & 3); }
        constexpr int BlueY() const { return (blueHighBitsY << 2) | ((blueWhiteLowBits >> 4) & 3); }
        constexpr int WhiteX() const { return (whiteHighBitsX << 2) | ((blueWhiteLowBits >> 2) & 3); }
        constexpr int WhiteY() const { return (whiteHighBitsY << 2) | ((blueWhiteLowBits >> 0) & 3); }

        // Discover all available video modes. The preferred video modex index can optionally be returned in the parameter. If the
        // preferred video mode cannot be determined, -1 will be returned as the index.
        mtl::vector<VideoMode> DiscoverModes(int* preferredVideoModeIndex) const;
    };

    static_assert(sizeof(EdidDataBlock) == 128, "EDID data block should be 128 bytes long");
} // namespace mtl
