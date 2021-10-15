/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <metal/unicode.hpp>


namespace metal {


long utf8_to_codepoint(const char8_t*& src, const char8_t* end)
{
    long codepoint = -1;

    if (src < end)
    {
        if (src[0] < 0x80)
        {
            codepoint = src[0];
            src += 1;
        }
        else if ((src[0] & 0xe0) == 0xc0)
        {
            if (src + 2 <= end)
            {
                codepoint = ((src[0] & 0x1f) << 6)
                          | ((src[1] & 0x3f) << 0);
                src += 2;
            }
            else
            {
                src = end;
            }
        }
        else if ((src[0] & 0xf0) == 0xe0)
        {
            if (src + 3 <= end)
            {
                codepoint = ((src[0] & 0x0f) << 12)
                          | ((src[1] & 0x3f) << 6)
                          | ((src[2] & 0x3f) << 0);
                src += 3;
            }
            else
            {
                src = end;
            }
        }
        else if ((src[0] & 0xf8) == 0xf0 && (src[0] <= 0xf4))
        {
            if (src + 4 <= end)
            {
                codepoint = ((src[0] & 0x07) << 18)
                          | ((src[1] & 0x3f) << 12)
                          | ((src[2] & 0x3f) << 6)
                          | ((src[3] & 0x3f) << 0);
                src += 4;
            }
            else
            {
                src = end;
            }
        }
        else
        {
            // Invalid
            src += 1;
        }
    }

    return codepoint;
}


} // namespace metal
