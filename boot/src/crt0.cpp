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

#include "boot.hpp"


using constructor_t = void (*)();

/*
    By default, the compiler will put all global constructors in a .CRT$XCU section. The
    linker will then alphabetically sort all the .CRT$xxx sections in a final .CRT section.
    To determine the start and end of the constructor array, we define two objects and
    put them in .CRT$XCA and .CRT$XCZ. They will end up as the first and last object in
    the executable's .CRT section.
*/

static const constructor_t* const __init_array_start __attribute__((section(".CRT$XCA"))) = (constructor_t*)&__init_array_start + 1;
static const constructor_t* const __init_array_end   __attribute__((section(".CRT$XCZ"))) = (constructor_t*)&__init_array_end;


static void _init()
{
    for (auto constructor = __init_array_start; constructor < __init_array_end; ++constructor)
    {
        (*constructor)();
    }
}


extern "C" EFIAPI efi::Status _start(efi::Handle hImage, efi::SystemTable* systemTable)
{
    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->bootServices;
    g_efiRuntimeServices = systemTable->runtimeServices;

    _init();

    return efi_main();
}
