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

#ifndef _RAINBOW_BOOT_EFIDISPLAY_HPP
#define _RAINBOW_BOOT_EFIDISPLAY_HPP

#include <memory>

#include <rainbow/uefi.h>
#include <Protocol/EdidActive.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/GraphicsOutput.h>

#include <graphics/display.hpp>


/*
    With EFI_GRAPHICS_OUTPUT_PROTOCOL, there is no guarantee that one can access
    the framebuffer directly. For example, this is not possible when using QEMU
    and emulating ARM or AARCH64 with the virt machine. This might also happen
    with real hardware. The proper way of handling this is to use the Blt()
    method. This can also be faster than copying pixels manually  if the
    implementation uses DMA or other tricks.
*/
class EfiDisplay : public IDisplay
{
public:

    EfiDisplay(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, EFI_EDID_ACTIVE_PROTOCOL* edid);

    EfiDisplay(EfiDisplay&& other);


private:

    // IDisplay
    int GetModeCount() const override;
    void GetCurrentMode(GraphicsMode* mode) const override;
    bool GetMode(int index, GraphicsMode* mode) const override;
    bool SetMode(int index) override;
    Surface* GetBackbuffer() override;
    void Blit(int x, int y, int width, int height) override;
    bool GetFramebuffer(Framebuffer* framebuffer) override;
    bool GetEdid(Edid* edid) const override;
    SimpleDisplay* ToSimpleDisplay() override;

    void InitBackbuffer();

    EFI_GRAPHICS_OUTPUT_PROTOCOL* m_gop;    // Can't be null
    EFI_EDID_ACTIVE_PROTOCOL* m_edid;       // Can be null

    std::unique_ptr<Surface> m_backbuffer;
};


#endif
