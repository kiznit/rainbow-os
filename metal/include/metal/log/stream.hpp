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

#pragma once

#include <cstdint>
#include <metal/log/core.hpp>
#include <metal/static_vector.hpp>
#include <utility>

namespace metal
{
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

        // Write data to the stream, returns how many characters were written
        void Write(const char8_t* text, size_t length);
        void Write(const wchar_t* text, size_t length);
        void Write(unsigned long value, bool negative);
        void Write(unsigned long long value, bool negative);
        void Write(const void* value);

        void Write(char8_t c) { m_buffer.push_back(c); }
        void Write(wchar_t c) { Write(&c, 1); }

        void WriteHex(unsigned long value);
        void WriteHex(unsigned long long value);

    private:
        LogRecord& m_record;
        metal::static_vector<char8_t, 200> m_buffer;
    };

    // Not implemented for now
    inline LogStream& operator<<(LogStream& stream, char value);
    inline LogStream& operator<<(LogStream& stream, signed char value);
    inline LogStream& operator<<(LogStream& stream, unsigned char value);
    inline LogStream& operator<<(LogStream& stream, const char* value);

    inline LogStream& operator<<(LogStream& stream, char8_t c)
    {
        stream.Write(c);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, wchar_t c)
    {
        stream.Write(c);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, std::u8string_view text)
    {
        stream.Write(text.data(), text.length());
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, std::wstring_view text)
    {
        stream.Write(text.data(), text.length());
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, const char8_t* text)
    {
        return stream << std::u8string_view(text);
    }

    inline LogStream& operator<<(LogStream& stream, const wchar_t* text)
    {
        return stream << std::wstring_view(text);
    }

    inline LogStream& operator<<(LogStream& stream, int value)
    {
        stream.Write((unsigned long)(value < 0 ? 0 - value : value), value < 0);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, unsigned int value)
    {
        stream.Write((unsigned long)value, false);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, long value)
    {
        stream.Write((unsigned long)(value < 0 ? 0 - value : value), value < 0);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, unsigned long value)
    {
        stream.Write((unsigned long)value, false);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, long long value)
    {
        stream.Write((unsigned long long)(value < 0 ? 0 - value : value), value < 0);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, unsigned long long value)
    {
        stream.Write((unsigned long long)value, false);
        return stream;
    }

    inline LogStream& operator<<(LogStream& stream, const void* value)
    {
        stream.Write(value);
        return stream;
    }

    class LogMagic
    {
    public:
        LogMagic(LogSystem& logSystem, LogRecord& record) : m_logSystem(logSystem), m_stream(record)
        {
        }

        ~LogMagic() { m_logSystem.PushRecord(std::move(m_stream.GetRecord())); }

        LogStream& GetStream() { return m_stream; }

    private:
        LogSystem& m_logSystem;
        LogStream m_stream;
    };

// We use LogMagic and a for() loop to give scope to a stream expression such as "stream << a << b
// << c". After the first iteration of the loop, LogMagic's destructor will be called. This will
// flush the stream and send the record to the logging system.
#define METAL_LOG(SEVERITY)                                                                        \
    for (metal::LogRecord record = metal::g_log.CreateRecord(metal::SEVERITY); !record.valid;)     \
    metal::LogMagic(metal::g_log, record).GetStream()

} // namespace metal