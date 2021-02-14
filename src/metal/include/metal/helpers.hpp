/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_METAL_HELPERS_HPP
#define _RAINBOW_METAL_HELPERS_HPP

#include <cstddef>
#include <cstdint>


#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)


// Helpers
template<typename T>
T* advance_pointer(T* p, intptr_t delta)
{
    return (T*)((uintptr_t)p + delta);
}


template<typename T>
T* align_down(T* p, uintptr_t alignment)
{
    return (T*)((uintptr_t)p & ~(alignment - 1));
}


template<typename T>
T align_down(T v, uintptr_t alignment)
{
    return (v) & ~(T(alignment) - 1);
}


template<typename T>
T* align_up(T* p, uintptr_t alignment)
{
    return (T*)(((uintptr_t)p + alignment - 1) & ~(alignment - 1));
}


template<typename T>
T align_up(T v, uintptr_t alignment)
{
    return (v + T(alignment) - 1) & ~(T(alignment) - 1);
}


template<typename T>
inline bool is_aligned(T* p, uintptr_t alignment)
{
    return ((uintptr_t)p & (alignment - 1)) == 0;
}


#endif
