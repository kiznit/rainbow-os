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

#include <algorithm>
#include <cstring>
#include <metal/log/stream.hpp>
#include <type_traits>

namespace mtl
{
    LogStream::LogStream(LogRecord& record) : m_record(record) {}

    void LogStream::Flush()
    {
        // TODO: buffer needs to be copied to the record, a string_view won't do here
        m_record.message = std::u8string_view{m_buffer.data(), m_buffer.size()};
        m_record.valid = true;

        m_buffer.clear();
    }

    void LogStream::Write(const char8_t* text, size_t length)
    {
        while (length--)
        {
            m_buffer.push_back(*text++);
        }
    }

    void LogStream::Write(const char16_t* text, size_t length)
    {
        // TODO: here we assume UCS-2 (UEFI) or UTF-32. If some future platform
        //       uses UTF-16, we will need to revisit and handle surrogate pairs.
        //       We might also have to fix this since NTFS uses UTF-16 for filenames
        //       and I doubt UEFI will handle this at all (not that it can).

        // TODO: move this to metal?

        while (length--)
        {
            const auto codepoint = *text++;

            if (codepoint < 0x80)
            {
                const char8_t c0 = codepoint;
                Write(c0);
            }
            else if (codepoint < 0x800)
            {
                const char8_t c0 = 0xC0 | (codepoint >> 6);
                const char8_t c1 = 0x80 | (codepoint & 0x3F);
                Write(c0);
                Write(c1);
            }
            else
            {
                const char8_t c0 = 0xE0 | (codepoint >> 12);
                const char8_t c1 = 0x80 | ((codepoint >> 6) & 0x3F);
                const char8_t c2 = 0x80 | (codepoint & 0x3F);
                Write(c0);
                Write(c1);
                Write(c2);
            }
        }
    }

    template <typename T>
    static void WriteImpl(LogStream& stream, T value, bool negative, const int base = 10,
                          const int width = 0)
    {
        static constexpr int MAX_DIGITS = 32; // Enough space for 20 digits (2^64) - TODO:
                                              // std::numeric_limits<unsigned long>::digits10]?

        if (negative && value)
        {
            stream.Write(u8'-');
        }

        char8_t digits[MAX_DIGITS];
        int count = 0;

        do
        {
            const auto digit = value % base;
            digits[count++] = digit < 10 ? u8'0' + digit : u8'a' + digit - 10;
            value /= base;
        } while (value && count < MAX_DIGITS);

        for (int leadingZeroes = width - count; leadingZeroes > 0; --leadingZeroes)
        {
            stream.Write(u8'0');
        }

        for (auto i = count; i > 0; --i)
        {
            stream.Write(digits[i - 1]);
        }
    }

    void LogStream::Write(unsigned long value, bool negative) { WriteImpl(*this, value, negative); }

    void LogStream::Write(unsigned long long value, bool negative)
    {
        WriteImpl(*this, value, negative);
    }

    void LogStream::Write(const void* value)
    {
        WriteImpl(*this, (uintptr_t)value, false, 16, sizeof(void*) * 2);
    }

} // namespace mtl
