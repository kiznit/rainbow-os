/*
    Copyright (c) 2022, Thierry Tremblay
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

#include "IDisplay.hpp"

namespace mtl
{
    /*
        Simple display implementation for when the framebuffer is directly accessible.
        Modes and EDID functionality are not available unless you subclass SimpleDisplay.
    */

    class SimpleDisplay : public IDisplay
    {
    public:
        SimpleDisplay(std::shared_ptr<Surface> framebuffer) : SimpleDisplay(framebuffer, framebuffer) {}
        SimpleDisplay(std::shared_ptr<Surface> frontbuffer, std::shared_ptr<Surface> backbuffer);

    protected:
        // IDisplay
        int GetModeCount() const override;
        void GetCurrentMode(GraphicsMode* mode) const override;
        bool GetMode(int index, GraphicsMode* mode) const override;
        bool SetMode(int index) override;
        std::shared_ptr<Surface> GetFrontbuffer() override;
        std::shared_ptr<Surface> GetBackbuffer() override;
        void Blit(int x, int y, int width, int height) override;
        // bool GetEdid(Edid* edid) const override;

        std::shared_ptr<Surface> m_frontbuffer;
        std::shared_ptr<Surface> m_backbuffer;
    };
} // namespace mtl