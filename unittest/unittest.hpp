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

#pragma once

#include <catch2/catch.hpp>
#include <trompeloeil/include/catch2/trompeloeil.hpp>

namespace Catch
{
    template <>
    struct StringMaker<char8_t>
    {
        static std::string convert(uint8_t value) { return StringMaker<uint8_t>::convert(value); }
    };

    template <>
    struct StringMaker<char16_t>
    {
        static std::string convert(uint16_t value) { return StringMaker<uint16_t>::convert(value); }
    };

    template <>
    struct StringMaker<char32_t>
    {
        static std::string convert(uint32_t value) { return StringMaker<uint32_t>::convert(value); }
    };
} // namespace Catch

namespace trompeloeil
{
    inline auto eq(const char16_t* string)
    {
        return make_matcher<const char16_t*>(
            [](const char16_t* value, const char16_t* expected) -> bool {
                // TODO: replace with a char16_t strcmp()
                for (; *value && *expected; ++value, ++expected)
                {
                    if (*value != *expected)
                    {
                        return false;
                    }
                }
                return true;
            },
            [](std::ostream& os, const char16_t* expected) {
                os << " does not match \"";
                print(os, expected);
                os << '"';
            },
            string);
    }
} // namespace trompeloeil
