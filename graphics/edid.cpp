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

#include "edid.hpp"
#include <cstring>
#include <metal/helpers.hpp>
#include <metal/log.hpp>


static const VideoMode s_establishedTimingModes[17] =
{
    { 720, 400, 70 },
    { 720, 400, 88 },
    { 640, 480, 60 },
    { 640, 480, 67 },
    { 640, 480, 72 },
    { 640, 480, 75 },
    { 800, 600, 56 },
    { 800, 600, 60 },
    { 800, 600, 72 },
    { 800, 600, 75 },
    { 832, 624, 75 },
    { 1024, 768, 87 },  // Interlaced
    { 1024, 768, 60 },
    { 1024, 768, 70 },
    { 1024, 768, 75 },
    { 1280, 1024, 75 },
    { 1152, 870, 75 }
};



Edid::Edid()
{
    m_size = 0;
    m_modeCount = 0;
    m_preferredMode = nullptr;
}



bool Edid::Initialize(const uint8_t* data, size_t size)
{
    if (!data)
        size = 0;
    else if (size > sizeof(m_data))
        size = sizeof(m_data);

    memcpy(m_data, data, size);
    m_size = size;
    m_modeCount = 0;
    m_preferredMode = nullptr;

    if (!Valid())
    {
        return false;
    }

    DiscoverModes();
    return true;
}



bool Edid::Valid() const
{
    // Minimum size
    if (m_size < 128)
    {
        return false;
    }

    // Header
    if (m_data[0] != 0x00 || m_data[1] != 0xFF || m_data[2] != 0xFF || m_data[3] != 0xFF ||
        m_data[4] != 0xFF || m_data[5] != 0xFF || m_data[6] != 0xFF || m_data[7] != 0x00)
    {
        return false;
    }

    // Checksum
    uint32_t checksum = 0;
    for (int i = 0; i != 128; ++i)
    {
        checksum += m_data[i];
    }

    if (checksum & 0xFF)
    {
        return false;
    }

    return true;
}



// TODO: support GTF modes, see Section 5 of the EDID spec at http://read.pudn.com/downloads110/ebook/456020/E-EDID%20Standard.pdf
void Edid::DiscoverModes()
{
    // Start with detailed timing modes (descriptors), first one is likely the preferred mode
    for (int i = 0; i != 4; ++i)
    {
        if (m_edid.detailedTimings[i][0] == 0 && m_edid.detailedTimings[i][1] == 0)
        {
            // TODO: handle 0xFA (additional standard timing), other types, ...
            // const uint8_t descriptorType = m_edid.detailedTimings[i][3];
        }
        else
        {
            // Skip interlaced modes as we don't know what to do with them at this time
            const bool interlaced = (m_edid.detailedTimings[i][17] & 0x80) ? true : false;
            if (interlaced)
            {
                continue;
            }

            // We have a detailed timing descriptor
            const int width  = (m_edid.detailedTimings[i][2]) | ((m_edid.detailedTimings[i][4] & 0xF0) << 4);
            const int height = (m_edid.detailedTimings[i][5]) | ((m_edid.detailedTimings[i][7] & 0xF0) << 4);

            // Calculate the refresh rate
            const uint32_t pclk = (m_edid.detailedTimings[i][1] << 8) | m_edid.detailedTimings[i][0];

            const uint32_t hactive = m_edid.detailedTimings[i][2] | ((m_edid.detailedTimings[i][4] & 0xF0) << 4);
            const uint32_t hblank  = m_edid.detailedTimings[i][3] | ((m_edid.detailedTimings[i][4] & 0x0F) << 8);
            const uint32_t htotal = hactive + hblank;

            const uint32_t vactive = m_edid.detailedTimings[i][5] | ((m_edid.detailedTimings[i][7] & 0xF0) << 4);
            const uint32_t vblank  = m_edid.detailedTimings[i][6] | ((m_edid.detailedTimings[i][7] & 0x0F) << 8);
            const uint32_t vtotal = vactive + vblank;

            const uint32_t totalPixels = htotal * vtotal;
            const uint32_t refreshRate = (pclk * 10000 + totalPixels / 2) / totalPixels;

            AddVideoMode(VideoMode(width, height, refreshRate));

            // For EDID 1.3 and above, the first detailed timing descriptor contains the preferred timing mode.
            // For older versions, we need to check if EDID_FEATURES_PREFERRED_TIMING_MODE is set on the features field.
            if (i == 0)
            {
                if (!(Version() == 1 && Revision() < 3) || m_edid.features & EDID_FEATURES_PREFERRED_TIMING_MODE)
                {
                    m_preferredMode = &m_modes[m_modeCount-1];
                }
            }
        }
    }

    // Standard timings
    for (int i = 0; i != 8; ++i)
    {
        const unsigned id = (m_edid.standardTimings[i*2] << 8) | (m_edid.standardTimings[i*2+1]);

        if (id != 0x0101)
        {
            AddStandardTimingMode(id);
        }
    }

    // Established timings
    const int supported = (m_edid.establishedTimings[0] << 9) | (m_edid.establishedTimings[1] << 1) | (m_edid.establishedTimings[2] >> 7);
    for (int i = 0; i != 17; ++i)
    {
        // Skip interlaced modes as we don't know what to do with them at this time
        if (i == 5)
        {
            continue;
        }

        if (supported & (1 << i))
        {
            AddVideoMode(s_establishedTimingModes[16 - i]);
        }
    }
}


void Edid::AddStandardTimingMode(uint16_t standardTiming)
{
    int width = (standardTiming >> 8) * 8 + 248;
    int ratio = (standardTiming & 0xFF) >> 6;
    int height = 0;

    switch (ratio)
    {
    case 0:
        if (Version() == 1 && Revision() < 3)
            height = width;
        else
            height = width * 10 / 16;
        break;

    case 1:
        height = width * 3 / 4;
        break;

    case 2:
        height = width * 4 / 5;
        break;

    case 3:
        height = width * 9 / 16;
        break;
    }

    int refreshRate = (standardTiming & 0x3f) + 60;

    AddVideoMode(VideoMode(width, height, refreshRate));
}



void Edid::AddVideoMode(const VideoMode& mode)
{
    if (m_modeCount == ARRAY_LENGTH(m_modes))
    {
        return;
    }

    // Check if we already know this mode
    for (int i = 0; i != m_modeCount; ++i)
    {
        if (m_modes[i] == mode)
        {
            return;
        }
    }

    m_modes[m_modeCount++] = mode;
}



void Edid::Dump() const
{
    Log("EDID Dump:\n");
    Log("    sizeof(edid)...: %ld\n", sizeof(m_edid));
    Log("    Valid..........: %d\n", Valid());

    uint16_t manufacturer = (m_data[8] << 8) | m_data[9];
    char m1 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;
    char m2 = ((manufacturer >> 5) & 0x1F) + 'A' - 1;
    char m3 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;

    Log("    Manufacturer ID: %c%c%c\n", m1, m2, m3);
    Log("    Serial.........: %x\n", Serial());

    Log("    EDID Version...: %d\n", Version());
    Log("    EDID Revision..: %d\n", Revision());

    Log("    Extensions.....: %d\n", m_data[126]);
    Log("    Gamma......... : %d\n", Gamma());

    Log("    CIE Red        : %d, %d\n", RedX(), RedY());
    Log("    CIE Green      : %d, %d\n", GreenX(), GreenY());
    Log("    CIE Blue       : %d, %d\n", BlueX(), BlueY());
    Log("    CIE White      : %d, %d\n", WhiteX(), WhiteY());

    Log("Supported modes:\n");
    for (int i = 0; i != m_modeCount; ++i)
    {
        Log("    %d x %d x %d\n", m_modes[i].width, m_modes[i].height, m_modes[i].refreshRate);
    }
}
