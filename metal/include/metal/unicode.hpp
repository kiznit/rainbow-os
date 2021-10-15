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

#include <cassert>


namespace metal {

// Unicode
//
// Interval	Description
// U+0000 - U+001F	    Control characters
// U+007F - U+009F	    Control characters
// U+DB00 - U+DFFF	    Surrogate pairs
// U+E000 - U+F8FF	    Private use area
// U+F0000 - U+FFFFF	Private use area
// U+100000 - U+10FFFF	Private use area


// Convert a utf-8 sequence into a unicode codepoint.
// Returns < 0 on error.
//
// The "src" pointer will be updated on both success and error to point to the
// start of the next utf-8 sequence (or the end).
//
// Progress will be made on both success and errors to ensure the caller can
// use a simple loop to walk a utf-8 string and convert it to codepoints.
//
// This function is secure in that it will not access memory at "end" or pass it.
//
// Note: if surrogate halves are found in the utf-8 stream, they will be returned
// as-is. This is not expected and if it happens it means the utf-8 stream is
// ill-formed. But not treating this as an error seems more robust, The caller can
// decide whether or not to merge surrogate pairs into valid codepoints.
long utf8_to_codepoint(const char8_t*& src, const char8_t* end);


// Convert a codepoint to a surrogate pair (UTF-16)
inline void codepoint_to_surrogates(long codepoint, char16_t& lead, char16_t& trail)
{
    assert(codepoint >= 0x010000 && codepoint <= 0x10FFFF);

    static constexpr long LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
    lead = LEAD_OFFSET + (codepoint >> 10);
    trail = 0xDC00 + (codepoint & 0x3FF);
}


// Convert a surrogate pair to a codepoint (UTF-16)
inline long surrogates_to_codepoint(char16_t lead, char16_t trail)
{
    assert(lead >= 0xD800 && lead <= 0xDBFF);
    assert(trail >= 0xDC00 && trail <= 0xDFFF);

    static constexpr long SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;
    long codepoint = ((long)lead << 10) + (long)trail + SURROGATE_OFFSET;
    return codepoint;
}


// Return whether or not the codepoint is valid for UCS-2
inline bool is_valid_ucs2_codepoint(long codepoint)
{
    return (codepoint >= 0 && codepoint < 0xDAFF) || (codepoint >= 0xF900 && codepoint <= 0xFFFF);
}



} // namespace metal
