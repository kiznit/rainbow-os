/*
    Copyright (c) 2024, Thierry Tremblay
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
#include <cassert>
#include <cstring>
#include <metal/log/stream.hpp>
#include <metal/unicode.hpp>
#include <type_traits>

namespace mtl
{
    LogStream::LogStream(LogRecord& record) : m_record(record)
    {
    }

    void LogStream::Flush()
    {
        m_record.message.assign(m_buffer.data(), m_buffer.size());
        m_record.valid = true;

        m_buffer.clear();
    }

    void LogStream::Write(mtl::string_view text)
    {
        for (auto c : text)
            m_buffer.push_back(c);
    }

    void LogStream::Write(mtl::u8string_view text)
    {
        for (auto c : text)
            m_buffer.push_back(c);
    }

    void LogStream::Write(mtl::u16string_view text)
    {
        // This is not the most efficient thing as we are creating
        // a temporary std::u8string when we could be writing directly
        // to the output buffer. At time of writing, this function is
        // only used in the bootloader, so we can live with it for now.
        const auto string = ToU8String(text);
        Write(string);
    }

    namespace
    {
        template <typename T>
        void WriteImpl(LogStream& stream, T value, bool negative, const int base = 10, const int width = 0)
        {
            constexpr int kMaxDigits = 32; // Enough space for 20 digits (2^64)

            if (negative && value)
            {
                stream.Write(u8'-');
            }

            char8_t digits[kMaxDigits];
            int count = 0;

            do
            {
                const auto digit = value % base;
                digits[count++] = digit < 10 ? u8'0' + digit : u8'a' + digit - 10;
                value /= base;
            } while (value && count < kMaxDigits);

            for (int leadingZeroes = width - count; leadingZeroes > 0; --leadingZeroes)
            {
                stream.Write(u8'0');
            }

            for (auto i = count; i > 0; --i)
            {
                stream.Write(digits[i - 1]);
            }
        }
    } // namespace

    void LogStream::Write(uint32_t value, bool negative)
    {
        WriteImpl(*this, value, negative);
    }

    void LogStream::Write(uint64_t value, bool negative)
    {
        WriteImpl(*this, value, negative);
    }

    void LogStream::WriteHex(uint32_t value, std::size_t width)
    {
        WriteImpl(*this, value, false, 16, width);
    }

    void LogStream::WriteHex(uint64_t value, std::size_t width)
    {
        WriteImpl(*this, value, false, 16, width);
    }

} // namespace mtl
