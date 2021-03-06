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

#ifndef _RAINBOW_BOOT_VBEDISPLAY_HPP
#define _RAINBOW_BOOT_VBEDISPLAY_HPP

#include <cstdint>

#include <graphics/simpledisplay.hpp>

struct VbeInfo;
struct VbeMode;


class VbeDisplay : public SimpleDisplay
{
public:

    void Initialize(const Framebuffer& framebuffer);

private:

    // IDisplay
    int GetModeCount() const override;
    void GetCurrentMode(GraphicsMode* mode) const override;
    bool GetMode(int index, GraphicsMode* mode) const override;
    bool SetMode(int index) override;
    bool GetEdid(Edid* edid) const override;

    void InitBackbuffer();

    VbeInfo*        m_info;         // Buffer for VbeInfo
    VbeMode*        m_mode;         // Buffer for VbeMode
    int             m_modeCount;    // How many modes are available
    const uint16_t* m_modes;        // List of available VESA modes
};


#endif
