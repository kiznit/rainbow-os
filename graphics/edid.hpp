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


#ifndef _RAINBOW_GRAPHICS_EDID_HPP
#define _RAINBOW_GRAPHICS_EDID_HPP

#include <stddef.h>
#include <stdint.h>


/*
    August 1994, DDC standard version 1 – EDID v1.0 structure.
    April 1996, EDID standard version 2 – EDID v1.1 structure.
    1997, EDID standard version 3 – EDID structures v1.2 and v2.0
    February 2000, E-EDID Standard Release A, v1.0 – EDID structure v1.3, EDID structure v2.0 deprecated
    September 2006 – E-EDID Standard Release A, v2.0 – EDID structure v1.4
*/

#define EDID_FEATURES_PREFERRED_TIMING_MODE 0x02


// EDID Data Block version 1.x
struct EdidDataBlock
{
    // Header
    uint8_t header[8];                  // 00 FF FF FF FF FF FF 00

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
    uint8_t maxHorizontalImageSize;     // in cm
    uint8_t maxVerticalImageSize;       // in cm
    uint8_t gamma;                      // (gamma * 100) - 100, range [1..3.54]
    uint8_t features;

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
    uint8_t detailedTimings[4][18]; // NOTE: EDID 1 and 2 allowed this space to be used for Monitor Descriptors

    // Trailer
    uint8_t extensionCount;
    uint8_t checksum;
};



struct VideoMode
{
    VideoMode() {}
    VideoMode(int w, int h, int r) : width(w), height(h), refreshRate(r) {}

    int width;
    int height;
    int refreshRate;
};


inline bool operator==(const VideoMode& lhs, const VideoMode& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height && lhs.refreshRate == rhs.refreshRate;
}

inline bool operator!=(const VideoMode& lhs, const VideoMode& rhs)
{
    return !(lhs == rhs);
}


class Edid
{
public:

    Edid();

    // Initialize with raw data and return whether or not the data is valid
    bool Initialize(const uint8_t* data, size_t size);

    // Is the Edid data valid?
    bool Valid() const;

    // Dump to stdout
    void Dump() const;

    // Accessors
    int Version() const             { return m_edid.version; }
    int Revision() const            { return m_edid.revision; }

    // Returns Gamma multipled by 100
    // TODO: if this is 0xFF, gamma is not defined here... find out where it is and return it (do not assume it is 2.2)
    int Gamma() const               { return m_edid.gamma == 0xFF ? 220 : m_edid.gamma + 100; }

    uint32_t Serial() const         { return (m_data[12] << 24) | (m_data[13] << 16) | (m_data[14] << 8) | m_data[15]; }

    bool HasSRGB() const            { return m_data[24] & 4; }

    // CIE xy coordinates [0..1023]
    int RedX() const                { return (m_edid.redHighBitsX << 2)   | ((m_edid.redGreenLowBits  >> 6) & 3); }
    int RedY() const                { return (m_edid.redHighBitsY << 2)   | ((m_edid.redGreenLowBits  >> 4) & 3); }
    int GreenX() const              { return (m_edid.greenHighBitsX << 2) | ((m_edid.redGreenLowBits  >> 2) & 3); }
    int GreenY() const              { return (m_edid.greenHighBitsY << 2) | ((m_edid.redGreenLowBits  >> 0) & 3); }
    int BlueX() const               { return (m_edid.blueHighBitsX << 2)  | ((m_edid.blueWhiteLowBits >> 6) & 3); }
    int BlueY() const               { return (m_edid.blueHighBitsY << 2)  | ((m_edid.blueWhiteLowBits >> 4) & 3); }
    int WhiteX() const              { return (m_edid.whiteHighBitsX << 2) | ((m_edid.blueWhiteLowBits >> 2) & 3); }
    int WhiteY() const              { return (m_edid.whiteHighBitsY << 2) | ((m_edid.blueWhiteLowBits >> 0) & 3); }


    // There might not be any preferred mode (i.e. this can return NULL).
    const VideoMode* GetPreferredMode() const { return m_preferredMode; }


private:

    void DiscoverModes();
    void AddVideoMode(const VideoMode& mode);
    void AddStandardTimingMode(uint16_t standardTiming);

    size_t  m_size;                     // Size of m_data

    union
    {
        uint8_t         m_data[128];    // EDID 2.0 defines a 256 bytes packet, so this is the max we support
        EdidDataBlock   m_edid;
    };


    // Established timing modes:   17
    // Standard timing modes:       8
    // Display descriptors:        24 (4 x FA descriptor with 6 entries)
    // Total:                      49
    int                 m_modeCount;        // How many modes are supported
    VideoMode           m_modes[64];        // List of supported modes
    const VideoMode*    m_preferredMode;    // Will be NULL if no preferred mode available

};


#endif
