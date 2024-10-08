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

#pragma once

#include <metal/shared_ptr.hpp>
#include <metal/string.hpp>
#include <metal/vector.hpp>

namespace mtl
{
    enum class LogSeverity
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    struct LogRecord
    {
        bool valid{false}; // Is the record valid? (TODO: would be nice to get rid of this field)
        // TODO: uint64_t            timestamp;
        LogSeverity severity;
        mtl::u8string message{}; // TODO: log record needs to own the message
    };

    class Logger
    {
    public:
        virtual ~Logger() = default;

        virtual void Log(const LogRecord& record) = 0;
    };

    class LogSystem
    {
    public:
        void AddLogger(mtl::shared_ptr<Logger> logger);
        void RemoveLogger(const mtl::shared_ptr<Logger>& logger);

        LogRecord CreateRecord(LogSeverity severity);

        void PushRecord(LogRecord&& record);

    private:
        mtl::vector<mtl::shared_ptr<Logger>> m_loggers;
    };

    extern LogSystem g_log;

} // namespace mtl