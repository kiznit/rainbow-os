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

#include <metal/unicode.hpp>
#include <vector>

namespace mtl
{
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
                    codepoint = ((src[0] & 0x1f) << 6) | ((src[1] & 0x3f) << 0);
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
                    codepoint = ((src[0] & 0x0f) << 12) | ((src[1] & 0x3f) << 6) | ((src[2] & 0x3f) << 0);
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
                    codepoint = ((src[0] & 0x07) << 18) | ((src[1] & 0x3f) << 12) | ((src[2] & 0x3f) << 6) | ((src[3] & 0x3f) << 0);
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

    std::u8string to_u8string(std::u16string_view string)
    {
        std::vector<char8_t> buffer;
        buffer.reserve(string.size());

        for (auto text = string.begin(); text != string.end();)
        {
            long codepoint = *text++;

            // Handle surrogate pairs
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
            {
                if (text != string.end())
                {
                    const auto trail = *text;

                    if (trail >= 0xDC00 && trail <= 0xDFFF)
                    {
                        codepoint = surrogates_to_codepoint(codepoint, trail);
                        ++text;
                    }
                    else
                    {
                        codepoint = 0xFFFD;
                    }
                }
                else
                {
                    codepoint = 0xFFFD;
                }
            }

            if (codepoint < 0x80)
            {
                buffer.push_back(codepoint);
            }
            else if (codepoint < 0x800)
            {
                buffer.push_back(0xC0 | (codepoint >> 6));
                buffer.push_back(0x80 | (codepoint & 0x3F));
            }
            else if (codepoint < 0x10000)
            {
                buffer.push_back(0xE0 | (codepoint >> 12));
                buffer.push_back(0x80 | ((codepoint >> 6) & 0x3F));
                buffer.push_back(0x80 | (codepoint & 0x3F));
            }
            else
            {
                buffer.push_back(0xF0 | (codepoint >> 18));
                buffer.push_back(0x80 | ((codepoint >> 12) & 0x3F));
                buffer.push_back(0x80 | ((codepoint >> 6) & 0x3F));
                buffer.push_back(0x80 | (codepoint & 0x3F));
            }
        }

        return std::u8string(buffer.begin(), buffer.end());
    }

    std::u16string to_u16string(std::u8string_view string, to_u16string_format format)
    {
        std::vector<char16_t> buffer;
        buffer.reserve(string.size());

        auto text = string.begin();
        auto end = string.end();
        while (text != end)
        {
            const auto codepoint = mtl::utf8_to_codepoint(text, end);

            if (format == Utf16)
            {
                if (codepoint <= 0xFFFF)
                {
                    buffer.push_back(codepoint);
                }
                else
                {
                    char16_t lead, trail;
                    codepoint_to_surrogates(codepoint, lead, trail);
                    buffer.push_back(lead);
                    buffer.push_back(trail);
                }
            }
            else if (format == Ucs2)
            {
                if (mtl::is_valid_ucs2_codepoint(codepoint))
                {
                    buffer.push_back(codepoint);
                }
                else
                {
                    buffer.push_back(u'\uFFFD');
                }
            }
            else
            {
                assert(!"Unknown format");
            }
        }

        return std::u16string(buffer.begin(), buffer.end());
    }
} // namespace mtl
