/*
    Copyright (c) 2023, Thierry Tremblay
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
#include <metal/log/core.hpp>
#include <metal/static_vector.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mtl
{
    using namespace std::literals;

    class LogStream
    {
    public:
        LogStream(LogRecord& record);

        LogRecord& GetRecord()
        {
            const_cast<LogStream*>(this)->Flush();
            return m_record;
        }

        void Flush();

        // Write data to the stream
        void Write(std::string_view text); // Assume 7-bit ASCII
        void Write(std::u8string_view text);
        void Write(std::u16string_view text);

        void Write(uint32_t value, bool negative);
        void Write(uint64_t value, bool negative);

        template <typename T>
            requires(std::is_integral_v<T>)
        constexpr void Write(T value)
        {
            if constexpr (std::is_same_v<T, char>)
            {
                m_buffer.push_back(value); // Assume 7-bit ASCII
            }
            else if constexpr (std::is_same_v<T, char8_t>)
            {
                m_buffer.push_back(value);
            }
            else if constexpr (std::is_same_v<T, char16_t>)
            {
                Write(std::u16string_view(&value, 1));
            }
            else if constexpr (std::is_same_v<T, wchar_t>)
            {
                // Unsupported
                static_assert(!std::is_same_v<T, wchar_t>);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                Write(value ? "true"sv : "false"sv);
            }
            else if constexpr (std::is_signed_v<T>)
            {
                if (sizeof(T) <= sizeof(uint32_t))
                    Write((uint32_t)(value < 0 ? 0 - value : value), value < 0);
                else
                    Write((uint64_t)(value < 0 ? 0 - value : value), value < 0);
            }
            else
            {
                if (sizeof(T) <= sizeof(uint32_t))
                    Write((uint32_t)value, false);
                else
                    Write((uint64_t)value, false);
            }
        }

        void WriteHex(uint32_t value, std::size_t width);
        void WriteHex(uint64_t value, std::size_t width);

        template <typename T>
        constexpr void WriteHex(T value)
        {
            if (sizeof(T) <= sizeof(uint32_t))
                WriteHex(static_cast<uint32_t>(value), sizeof(T) * 2);
            else
                WriteHex(static_cast<uint64_t>(value), sizeof(T) * 2);
        }

        template <typename T>
            requires(!std::is_same_v<T, char> && !std::is_same_v<T, char8_t> && !std::is_same_v<T, char16_t> &&
                     !std::is_same_v<T, wchar_t>)
        void Write(const T* value)
        {
            WriteHex(reinterpret_cast<uintptr_t>(value), sizeof(void*) * 2);
        }

    private:
        LogRecord& m_record;
        mtl::static_vector<char8_t, 200> m_buffer;
    };

    inline LogStream& operator<<(LogStream& stream, std::string_view text)
    {
        stream.Write(text);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, std::u8string_view text)
    {
        stream.Write(text);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, std::u16string_view text)
    {
        stream.Write(text);
        return stream;
    }

    template <typename T>
        requires(std::is_integral_v<T>)
    inline LogStream& operator<<(LogStream& stream, T value)
    {
        stream.Write<T>(value);
        return stream;
    }

    template <typename T>
        requires(!std::is_same_v<T, char> && !std::is_same_v<T, char8_t> && !std::is_same_v<T, char16_t> &&
                 !std::is_same_v<T, wchar_t>)
    inline LogStream& operator<<(LogStream& stream, const T* value)
    {
        stream.Write(value);
        return stream;
    }

    template <typename T>
    struct hex
    {
        hex(const T& value) : value(value) {}
        const T value;
    };

    template <typename T>
    inline LogStream& operator<<(LogStream& stream, const hex<T>& hex)
    {
        stream.WriteHex(hex.value);
        return stream;
    }

    class LogMagic
    {
    public:
        LogMagic(LogSystem& logSystem, LogRecord& record) : m_logSystem(logSystem), m_stream(record) {}

        ~LogMagic() { m_logSystem.PushRecord(std::move(m_stream.GetRecord())); }

        LogStream& GetStream() { return m_stream; }

    private:
        LogSystem& m_logSystem;
        LogStream m_stream;
    };

// We use LogMagic and a for() loop to give scope to a stream expression such as "stream << a << b
// << c". After the first iteration of the loop, LogMagic's destructor will be called. This will
// flush the stream and send the record to the logging system.
#define MTL_LOG(SEVERITY)                                                                                                          \
    for (mtl::LogRecord record = mtl::g_log.CreateRecord(mtl::LogSeverity::SEVERITY); !record.valid;)                              \
    mtl::LogMagic(mtl::g_log, record).GetStream()

} // namespace mtl
