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

#include "edid.hpp"
#include <metal/crt.hpp>
#include <metal/log.hpp>


static const uint8_t edid_example[] =
{
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x3A, 0xC4, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x2D, 0x0C, 0x01, 0x03, 0x80, 0x20, 0x18, 0x00, 0xEA, 0xA8, 0xE0, 0x99, 0x57, 0x4B, 0x92, 0x25,
    0x1C, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x48, 0x3F, 0x40, 0x30, 0x62, 0xB0, 0x32, 0x40, 0x4C, 0xC0,
    0x13, 0x00, 0x42, 0xF3, 0x10, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4E, 0x76, 0x69,
    0x64, 0x69, 0x61, 0x20, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x74,
    0x20, 0x46, 0x6C, 0x61, 0x74, 0x20, 0x50, 0x61, 0x6E, 0x65, 0x6C, 0x00, 0x00, 0x00, 0x00, 0xFD,
    0x00, 0x00, 0x3C, 0x1D, 0x4C, 0x11, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x9C
};


Edid::Edid()
{
    memcpy(m_data, edid_example, sizeof(edid_example));
    m_size = sizeof(edid_example);
}



Edid::Edid(const uint8_t* data, size_t size)
{
    if (!data)
        size = 0;
    else if (size > sizeof(m_data))
        size = sizeof(m_data);

    memcpy(m_data, data, size);
    m_size = size;
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



void Edid::Dump()
{
    Log("EDID Dump:\n");
    Log("    sizeof(edid)...: %d\n", (int)sizeof(m_edid));
    Log("    Valid..........: %d\n", Valid());

    uint16_t manufacturer = (m_data[8] << 8) | m_data[9];
    char m1 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;
    char m2 = ((manufacturer >> 5) & 0x1F) + 'A' - 1;
    char m3 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;

    Log("    Manufacturer ID: %c%c%c\n", m1, m2, m3);
    Log("    Serial.........: %x\n", (int)Serial());

    Log("    EDID Version...: %d\n", Version());
    Log("    EDID Revision..: %d\n", Revision());

    Log("    Extensions.....: %d\n", m_data[126]);
    Log("    Gamma......... : %d\n", Gamma());

    Log("    CIE Red        : %d, %d\n", RedX(), RedY());
    Log("    CIE Green      : %d, %d\n", GreenX(), GreenY());
    Log("    CIE Blue       : %d, %d\n", BlueX(), BlueY());
    Log("    CIE White      : %d, %d\n", WhiteX(), WhiteY());

    EnumerateDisplayModes(NULL, NULL);
}



struct VideoMode
{
    int width;
    int height;
    int refreshRate;
};


static const VideoMode s_videoModes[17] =
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



void Edid::EnumerateDisplayModes(EnumModeCallback callback, void* user)
{
    // Established timings (to be ignored?)
    const int supported = (m_edid.establishedTimings[0] << 9) || (m_edid.establishedTimings[1] << 1) || (m_edid.establishedTimings[2] >> 7);
    Log("    Established timings: %x\n", supported);
    for (int i = 0; i != 17; ++i)
    {
        const VideoMode& mode = s_videoModes[i];
        if (supported & (1 << i))
        {
            Log("        Index %d, mask %x: %d x %d x %d - %d\n", i, 1 << i, mode.width, mode.height, mode.refreshRate, supported & (1 << i));
        }
    }

    // Standard timings
    Log("    Standard timings:\n");
    for (int i = 0; i != 8; ++i)
    {
        const unsigned id = (m_edid.standardTimings[i*2] << 8) | (m_edid.standardTimings[i*2+1]);

        if (id != 0x0101)
        {
            int width = (id >> 8) * 8 + 248;
            int ratio = (id & 0xFF) >> 6;
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

            int refreshRate = (id & 0x3f) + 60;

            Log("        ID: %x (%d x %d x %d)\n", id, width, height, refreshRate);
        }
    }

    // Detailed timings
    Log("    Detailed timings:\n");
    for (int i = 0; i != 4; ++i)
    {
        if (m_edid.detailedTimings[i][0] == 0 && m_edid.detailedTimings[i][1] == 0)
        {
            uint8_t descriptorType = m_edid.detailedTimings[i][3];
            Log("        Descriptor %d: type %x\n", i, descriptorType);
        }
        else if (i == 0 && (m_edid.features & EDID_FEATURES_PREFERRED_TIMING_MODE))
        {
            int width  = (m_edid.detailedTimings[i][2]) | ((m_edid.detailedTimings[i][4] & 0xF0) << 4);
            int height = (m_edid.detailedTimings[i][5]) | ((m_edid.detailedTimings[i][7] & 0xF0) << 4);
            int refreshRate = 0;
            Log("        Detailed Timing: %d x %d x %d\n", width, height, refreshRate);
        }
    }


    (void)callback;
    (void)user;
}
