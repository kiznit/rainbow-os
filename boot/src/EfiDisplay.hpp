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

#include <memory>
#include <metal/graphics/SimpleDisplay.hpp>
#include <rainbow/uefi.hpp>
#include <rainbow/uefi/edid.hpp>
#include <rainbow/uefi/graphics.hpp>
#include <vector>

/*
    With efi::GraphicsOutputProtocol, there is no guarantee that one can access the framebuffer
    directly. For example, this is not possible when using QEMU and emulating ARM or AARCH64 with
    the virt machine. This might also happen with real hardware. The proper way of handling this is
    to use the Blt() method. This can also be faster than copying pixels manually if the
    implementation uses DMA or other tricks.
*/

class EfiDisplay : public mtl::IDisplay
{
public:
    EfiDisplay(efi::GraphicsOutputProtocol* gop, efi::EdidProtocol* edid);

    // IDisplay
    int GetModeCount() const override;
    void GetCurrentMode(mtl::GraphicsMode* mode) const override;
    bool GetMode(int index, mtl::GraphicsMode* mode) const override;
    bool SetMode(int index) override;
    std::shared_ptr<mtl::Surface> GetBackbuffer() override;
    void Blit(int x, int y, int width, int height) override;
    // bool GetFramebuffer(Framebuffer* framebuffer) override;
    // bool GetEdid(mtl::Edid* edid) const override;
    mtl::SimpleDisplay* ToSimpleDisplay() override;

private:
    void InitBackbuffer();

    efi::GraphicsOutputProtocol* m_gop; // Can't be null
    efi::EdidProtocol* m_edid;          // Can be null

    std::shared_ptr<mtl::Surface> m_backbuffer;
};

std::vector<EfiDisplay> InitializeDisplays(efi::BootServices* bootServices);
