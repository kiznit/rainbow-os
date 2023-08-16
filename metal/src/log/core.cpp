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

#include <metal/log/core.hpp>

namespace mtl
{
    LogSystem g_log;

    void LogSystem::AddLogger(std::shared_ptr<Logger> logger)
    {
        m_loggers.push_back(std::move(logger));
    }

    void LogSystem::RemoveLogger(const std::shared_ptr<Logger>& logger)
    {
        for (size_t i = 0; i != m_loggers.size(); ++i)
        {
            if (m_loggers[i] == logger)
            {
                for (; i != m_loggers.size() - 1; ++i)
                {
                    m_loggers[i] = m_loggers[i + 1];
                }

                m_loggers.resize(m_loggers.size() - 1);
                break;
            }
        }
    }

    LogRecord LogSystem::CreateRecord(LogSeverity severity)
    {
        LogRecord record = {.severity = severity};

        return record;
    }

    void LogSystem::PushRecord(LogRecord&& record)
    {
        for (const auto& logger : m_loggers)
        {
            logger->Log(record);
        }
    }

} // namespace mtl