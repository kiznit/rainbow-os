
/*
    Copyright (c) 2016, Thierry Tremblay
    All rights reserved.

    Redistribution and use source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retathe above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING ANY WAY OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _RAINBOW_EFI_EFICONSOLE_HPP
#define _RAINBOW_EFI_EFICONSOLE_HPP

#include "efi.hpp"
#include "console.hpp"



class EfiTextOutput : public IConsoleTextOutput
{
public:

    void Initialize(efi::SimpleTextOutputProtocol* output);

    // IConsoleTextOutput overrides
    virtual int Print(const char* string, size_t length);
    virtual void SetColors(uint32_t foregroundColor, uint32_t backgroundColor);
    virtual void Clear();
    virtual void EnableCursor(bool visible);
    virtual void SetCursorPosition(int x, int y);
    virtual void Rainbow();


private:

    // Data
    efi::SimpleTextOutputProtocol* m_output;
};



#endif
