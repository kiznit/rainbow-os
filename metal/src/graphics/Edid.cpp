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

#include <algorithm>
#include <cstring>
#include <metal/graphics/Edid.hpp>
#include <metal/log.hpp>

namespace mtl
{
    bool EdidDataBlock::Valid() const
    {
        // Header
        if (header[0] != 0x00 || header[1] != 0xFF || header[2] != 0xFF || header[3] != 0xFF || header[4] != 0xFF ||
            header[5] != 0xFF || header[6] != 0xFF || header[7] != 0x00)
        {
            return false;
        }

        // Checksum
        auto data = reinterpret_cast<const uint8_t*>(this);
        uint32_t checksum = 0;
        for (int i = 0; i != 128; ++i)
        {
            checksum += data[i];
        }

        if (checksum & 0xFF)
        {
            return false;
        }

        return true;
    }

    namespace
    {
        void AddDetailedTimingModes(const EdidDataBlock& edid, mtl::vector<VideoMode>& videoModes, int* preferredVideoModeIndex)
        {
            for (int i = 0; i != 4; ++i)
            {
                if (edid.detailedTimings[i][0] == 0 && edid.detailedTimings[i][1] == 0)
                {
                    // TODO: handle 0xFA (additional standard timing), other types, ...
                    // const uint8_t descriptorType = edid.detailedTimings[i][3];
                }
                else
                {
                    // Skip interlaced modes as we don't know what to do with them at this time
                    if (edid.detailedTimings[i][17] & 0x80)
                        continue;

                    // We have a detailed timing descriptor
                    const int width = (edid.detailedTimings[i][2]) | ((edid.detailedTimings[i][4] & 0xF0) << 4);
                    const int height = (edid.detailedTimings[i][5]) | ((edid.detailedTimings[i][7] & 0xF0) << 4);

                    // Calculate the refresh rate
                    const uint32_t pclk = (edid.detailedTimings[i][1] << 8) | edid.detailedTimings[i][0];

                    const uint32_t hactive = edid.detailedTimings[i][2] | ((edid.detailedTimings[i][4] & 0xF0) << 4);
                    const uint32_t hblank = edid.detailedTimings[i][3] | ((edid.detailedTimings[i][4] & 0x0F) << 8);
                    const uint32_t htotal = hactive + hblank;

                    const uint32_t vactive = edid.detailedTimings[i][5] | ((edid.detailedTimings[i][7] & 0xF0) << 4);
                    const uint32_t vblank = edid.detailedTimings[i][6] | ((edid.detailedTimings[i][7] & 0x0F) << 8);
                    const uint32_t vtotal = vactive + vblank;

                    const uint32_t totalPixels = htotal * vtotal;
                    const uint32_t refreshRate = (pclk * 10000 + totalPixels / 2) / totalPixels;

                    videoModes.emplace_back(VideoMode{width, height, static_cast<int>(refreshRate)});

                    if (preferredVideoModeIndex)
                    {
                        // For EDID 1.3 and above, the first detailed timing descriptor contains the
                        // preferred timing mode. For older versions, we need to check if
                        // EdidFeatures::PreferredTimingMode is set on the features field.
                        if (edid.version > 1 || edid.revision >= 3)
                            *preferredVideoModeIndex = 0;
                        else if (edid.features & EdidFeatures::PreferredTimingMode)
                            *preferredVideoModeIndex = i;
                    }
                }
            }
        }

        void AddStandardTimings(const EdidDataBlock& edid, mtl::vector<VideoMode>& videoModes)
        {
            for (int i = 0; i != 8; ++i)
            {
                const uint16_t standardTiming = (edid.standardTimings[i * 2] << 8) | (edid.standardTimings[i * 2 + 1]);

                if (standardTiming == 0x0101)
                    continue;

                const int width = (standardTiming >> 8) * 8 + 248;
                const int ratio = (standardTiming >> 6) & 3;
                int height = 0;

                switch (ratio)
                {
                case 0:
                    if (edid.version == 1 && edid.version < 3)
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

                const int refreshRate = (standardTiming & 0x3f) + 60;

                videoModes.emplace_back(VideoMode{width, height, refreshRate});
            }
        }

        constexpr VideoMode kEstablishedTimingModes[17]{
            {720, 400, 70},  {720, 400, 88},  {640, 480, 60},  {640, 480, 67},   {640, 480, 72}, {640, 480, 75},
            {800, 600, 56},  {800, 600, 60},  {800, 600, 72},  {800, 600, 75},   {832, 624, 75}, {1024, 768, 87}, // Interlaced
            {1024, 768, 60}, {1024, 768, 70}, {1024, 768, 75}, {1280, 1024, 75}, {1152, 870, 75}};

        void AddEstablishedTimings(const EdidDataBlock& edid, mtl::vector<VideoMode>& videoModes)
        {
            // Established timings
            const int supportedTimings =
                (edid.establishedTimings[0] << 9) | (edid.establishedTimings[1] << 1) | (edid.establishedTimings[2] >> 7);

            for (int i = 0; i != 17; ++i)
            {
                // Skip interlaced modes as we don't know what to do with them at this time
                if (i == 11)
                    continue;

                if (supportedTimings & (1 << i))
                    videoModes.emplace_back(kEstablishedTimingModes[16 - i]);
            }
        }
    } // namespace

    mtl::vector<VideoMode> EdidDataBlock::DiscoverModes(int* preferredVideoModeIndex) const
    {
        if (preferredVideoModeIndex)
            *preferredVideoModeIndex = -1;

        mtl::vector<VideoMode> videoModes;

        // TODO: support GTF modes, see Section 5 of the EDID spec at
        // http://read.pudn.com/downloads110/ebook/456020/E-EDID%20Standard.pdf

        AddDetailedTimingModes(*this, videoModes, preferredVideoModeIndex);
        AddStandardTimings(*this, videoModes);
        AddEstablishedTimings(*this, videoModes);

        return videoModes;
    }

    /*
        // void Edid::Dump() const
        // {
        //     Log("EDID Dump:\n");
        //     Log("    sizeof(edid)...: %ld\n", sizeof(m_edid));
        //     Log("    Valid..........: %d\n", Valid());

        //     uint16_t manufacturer = (m_data[8] << 8) | m_data[9];
        //     char m1 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;
        //     char m2 = ((manufacturer >> 5) & 0x1F) + 'A' - 1;
        //     char m3 = ((manufacturer >> 10) & 0x1F) + 'A' - 1;

        //     Log("    Manufacturer ID: %c%c%c\n", m1, m2, m3);
        //     Log("    Serial.........: %x\n", Serial());

        //     Log("    EDID Version...: %d\n", Version());
        //     Log("    EDID Revision..: %d\n", Revision());

        //     Log("    Extensions.....: %d\n", m_data[126]);
        //     Log("    Gamma......... : %d\n", Gamma());

        //     Log("    CIE Red        : %d, %d\n", RedX(), RedY());
        //     Log("    CIE Green      : %d, %d\n", GreenX(), GreenY());
        //     Log("    CIE Blue       : %d, %d\n", BlueX(), BlueY());
        //     Log("    CIE White      : %d, %d\n", WhiteX(), WhiteY());

        //     Log("Supported modes:\n");

        //     for (const auto& mode : m_modes)
        //     {
        //         Log("    %d x %d x %d\n", mode.width, mode.height, mode.refreshRate);
        //     }
        // }
    */
} // namespace mtl
