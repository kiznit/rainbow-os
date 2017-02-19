/*
    Copyright (c) 2017, Thierry Tremblay
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

#ifndef _RAINBOW_LIBC_ENDIAN_H
#define _RAINBOW_LIBC_ENDIAN_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

//todo: this code assumes host is little endian

inline uint16_t htobe16(uint16_t x)     { return __builtin_bswap16(x); }
inline uint16_t htole16(uint16_t x)     { return x; }
inline uint16_t betoh16(uint16_t x)     { return x; }
inline uint16_t letoh16(uint16_t x)     { return __builtin_bswap16(x); }

inline uint32_t htobe32(uint32_t x)     { return __builtin_bswap32(x); }
inline uint32_t htole32(uint32_t x)     { return x; }
inline uint32_t betoh32(uint32_t x)     { return __builtin_bswap32(x); }
inline uint32_t letoh32(uint32_t x)     { return x; }

inline uint64_t htobe64(uint64_t x)     { return __builtin_bswap64(x); }
inline uint64_t htole64(uint64_t x)     { return x; }
inline uint64_t betoh64(uint64_t x)     { return __builtin_bswap64(x); }
inline uint64_t letoh64(uint64_t x)     { return x; }

// Linux compatiblity
inline uint16_t be16toh(uint16_t x)     { return betoh16(x); }
inline uint16_t le16toh(uint16_t x)     { return letoh16(x); }
inline uint32_t be32toh(uint32_t x)     { return betoh32(x); }
inline uint32_t le32toh(uint32_t x)     { return letoh32(x); }
inline uint64_t be64toh(uint64_t x)     { return betoh64(x); }
inline uint64_t le64toh(uint64_t x)     { return letoh64(x); }


#ifdef __cplusplus
}
#endif

#endif
