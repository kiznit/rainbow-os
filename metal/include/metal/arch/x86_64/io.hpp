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

#include <cstdint>

namespace mtl
{
    static inline void IoOut8(uint16_t port, uint8_t value) { asm volatile("outb %1, %0" : : "dN"(port), "a"(value)); }

    static inline void IoOut16(uint16_t port, uint16_t value) { asm volatile("outw %1, %0" : : "dN"(port), "a"(value)); }

    static inline void IoOut32(uint16_t port, uint32_t value) { asm volatile("outl %1, %0" : : "dN"(port), "a"(value)); }

    static inline uint8_t IoIn8(uint16_t port)
    {
        uint8_t ret;
        asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    static inline uint16_t IoIn16(uint16_t port)
    {
        uint16_t ret;
        asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    static inline uint32_t IoIn32(uint16_t port)
    {
        uint32_t ret;
        asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    static inline void IoWait()
    {
        // Port 0x80 is used for POST codes and is safe to use as a delay mechanism
        // We also don't care what we write to it, so just use AL.
        asm volatile("outb %al, $0x80");
    }

} // namespace mtl
