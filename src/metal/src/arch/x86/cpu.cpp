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

#include <metal/cpu.hpp>
#include <cassert>


void GdtDescriptor::SetKernelData32(uint32_t base, uint32_t size)
{
    uint32_t limit = size - 1;

    assert(limit <= 0xFFFFF);

    // Limit (15:0)
    this->limit = limit & 0xFFFF;

    // Base (15:0)
    this->base  = base & 0xFFFF;

    // P + DPL 0 + S + Data + Write + Base (23:16)
    this->flags1 = 0x9200 | ((base >> 16) & 0x00FF);

    // B (32 bits) + limit (19:16)
    this->flags2 = 0x0040 | ((base >> 16) & 0xFF00) | ((limit >> 16) & 0x000F);
}


void GdtDescriptor::SetUserData32(uint32_t base, uint32_t size)
{
    uint32_t limit = size - 1;

    assert(limit <= 0xFFFFF);

    // Limit (15:0)
    this->limit = limit & 0xFFFF;

    // Base (15:0)
    this->base  = base & 0xFFFF;

    // P + DPL 3 + S + Data + Write + Base (23:16)
    this->flags1 = 0xF200 | ((base >> 16) & 0x00FF);

    // B (32 bits) + limit (19:16)
    this->flags2 = 0x0040 | ((base >> 16) & 0xFF00) | ((limit >> 16) & 0x000F);
}
