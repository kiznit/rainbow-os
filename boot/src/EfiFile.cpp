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

#include "EfiFile.hpp"
#include <cassert>

static constexpr const char8_t* s_severityText[6] = {u8"Trace  : ", u8"Debug  : ", u8"Info   : ",
                                                     u8"Warning: ", u8"Error  : ", u8"Fatal  : "};

EfiFile::EfiFile(efi::FileProtocol* file) : m_file(file)
{
    assert(m_file);
}

EfiFile::~EfiFile()
{
    m_file->Close(m_file);
}

void EfiFile::Log(const mtl::LogRecord& record)
{
    Write(s_severityText[record.severity]);
    Write(record.message);
    Write(u8"\n");

    m_file->Flush(m_file);
}

std::expected<void, efi::Status> EfiFile::Write(std::u8string_view string)
{
    efi::uintn_t size = string.size();
    auto status = m_file->Write(m_file, &size, string.data());
    if (efi::Error(status))
    {
        return std::unexpected(status);
    }

    return {};
}
