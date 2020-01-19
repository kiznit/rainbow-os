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

#include "display.hpp"
#include <graphics/edid.hpp>


// Pick the highest resolution available without exceeding maxWidth x maxHeight
// Here we have no idea what the ideal pixel ratio should be :(
static void SetBestMode(Display& display, const int maxWidth, const int maxHeight)
{
    GraphicsMode bestMode;
    bestMode.width = 0;
    bestMode.height = 0;
    bestMode.format = PIXFMT_UNKNOWN;

    int bestIndex = -1;

    for (int i = 0; i != display.GetModeCount(); ++i)
    {
        GraphicsMode mode;
        if (!display.GetMode(i, &mode) || mode.format == PIXFMT_UNKNOWN)
        {
            continue;
        }

        if (mode.width > maxWidth || mode.height > maxHeight)
        {
            continue;
        }

        if (mode.width > bestMode.width || mode.height > bestMode.height)
        {
            bestIndex = i;
            bestMode = mode;
        }
        else if (mode.width == bestMode.width && mode.height == bestMode.height && GetPixelDepth(mode.format) > GetPixelDepth(bestMode.format))
        {
            bestIndex = i;
            bestMode = mode;
        }
    }

    if (bestIndex >= 0)
    {
        display.SetMode(bestIndex);
    }
}



// Pick the best resolution available based on edid information
static void SetBestMode(Display& display, const Edid& edid)
{
    // TODO: we can do better than this...
    auto preferredMode = edid.GetPreferredMode();
    if (preferredMode)
    {
        SetBestMode(display, preferredMode->width, preferredMode->height);
    }
}



void SetBestMode(Display& display)
{
    Edid edid;
    if (display.GetEdid(&edid))
    {
        SetBestMode(display, edid);
    }
    else
    {
        GraphicsMode mode;
        display.GetCurrentMode(&mode);

        if (mode.width > 0 && mode.height > 0)
        {
            SetBestMode(display, mode.width, mode.height);
        }
        else
        {
            SetBestMode(display, 640, 480);
        }
    }
}
