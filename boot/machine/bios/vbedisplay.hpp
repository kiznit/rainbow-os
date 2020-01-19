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

#ifndef _RAINBOW_BOOT_VBEDISPLAY_HPP
#define _RAINBOW_BOOT_VBEDISPLAY_HPP

#include <stdint.h>
#include "display.hpp"

struct VbeInfo;
struct VbeMode;


class VbeDisplay : public Display
{
public:

    VbeDisplay(const GraphicsMode& currentMode);

    // Return how many different modes are supported by the display
    virtual int GetModeCount() const;

    // Return the current mode
    virtual void GetCurrentMode(GraphicsMode* mode) const;

    // Get a display mode description
    virtual bool GetMode(int index, GraphicsMode* mode) const;

    // Change the display mode
    virtual bool SetMode(int index);

    // Get the display's EDID information
    virtual bool GetEdid(Edid* edid) const;


private:

    GraphicsMode    m_currentMode;  // Current mode
    VbeInfo*        m_info;         // Buffer for VbeInfo
    VbeMode*        m_mode;         // Buffer for VbeMode
    int             m_modeCount;    // How many modes are available
    const uint16_t* m_modes;        // List of available VESA modes
};


#endif
