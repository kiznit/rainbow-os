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
#include <string_view>
#include <unittest.hpp>

using namespace metal;
using namespace std::literals;

struct Utf8TestCase
{
    long codepoint;
    std::u8string_view utf8;
};

static constexpr Utf8TestCase s_utf8_valid_sequences[]{
    {0x00061, u8"a"sv},          // 1 byte
    {0x000E9, u8"é"sv},          // 2 bytes
    {0x02202, u8"∂"sv},          // 3 bytes
    {0x1F64A, u8"\U0001f64a"sv}, // 4-bytes, speak-no-evil
    {-1, u8""sv},                // Empty string
};

TEST_CASE("utf8_to_codepoint() - valid sequences", "[unicode]")
{
    for (const auto& entry : s_utf8_valid_sequences)
    {
        auto start = entry.utf8.cbegin();
        const auto end = entry.utf8.cend();
        const auto codepoint = utf8_to_codepoint(start, end);
        REQUIRE(codepoint == entry.codepoint);
        REQUIRE(start == end);
    }
}

// We can't use char8_t[] literals to define invalid UTF-8 sequences, so we use char[] literals
static constexpr std::string_view s_utf8_invalid_sequences[]{
    "\xC3"sv,         // 2 bytes sequence missing 1 byte
    "\xEF\x8F"sv,     // 3 bytes sequence missing 1 byte
    "\xEF"sv,         // 3 bytes sequence missing 2 bytes
    "\xF3\x8F\x8F"sv, // 4 bytes sequence missing 1 byte
    "\xF3\x8F"sv,     // 4 bytes sequence missing 2 bytes
    "\xF3"sv,         // 4 bytes sequence missing 3 bytes
};

TEST_CASE("utf8_to_codepoint() - invalid sequences", "[unicode]")
{
    for (const auto& entry : s_utf8_invalid_sequences)
    {
        auto start = (const char8_t*)entry.cbegin();
        const auto end = (const char8_t*)entry.cend();
        const auto codepoint = utf8_to_codepoint(start, end);
        REQUIRE(codepoint == -1);
        REQUIRE(start == end);
    }
}

struct SurrogatesTestCase
{
    long codepoint;
    char16_t lead;
    char16_t trail;
};

static constexpr SurrogatesTestCase s_surrogates_test_cases[]{
    {0x010000, 0xD800, 0xDC00},
    {0x010E6D, 0xD803, 0xDE6D},
    {0x01D11E, 0xD834, 0xDD1E},
    {0x10FFFF, 0xDBFF, 0xDFFF},
};

TEST_CASE("codepoint_to_surrogates()", "[unicode]")
{
    for (const auto& entry : s_surrogates_test_cases)
    {
        char16_t lead, trail;
        codepoint_to_surrogates(entry.codepoint, lead, trail);
        REQUIRE(lead == entry.lead);
        REQUIRE(trail == entry.trail);
    }
}

TEST_CASE("surrogates_to_codepoint()", "[unicode]")
{
    for (const auto& entry : s_surrogates_test_cases)
    {
        const auto codepoint = surrogates_to_codepoint(entry.lead, entry.trail);
        REQUIRE(codepoint == entry.codepoint);
    }
}
