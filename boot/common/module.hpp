/*
    Copyright (c) 2015, Thierry Tremblay
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

#ifndef INCLUDED_BOOT_COMMON_MODULE_HPP
#define INCLUDED_BOOT_COMMON_MODULE_HPP

#include <sys/types.h>


#define MODULE_MAX_ENTRIES 128
#define MODULE_MAX_NAME_LENGTH 64


struct ModuleInfo
{
    physaddr_t  start;
    physaddr_t  end;
    char        name[MODULE_MAX_NAME_LENGTH];
};



class Modules
{
public:

    Modules();

    void AddModule(const char* name, physaddr_t start, physaddr_t end);

    void Print();


    // STL interface
    typedef const ModuleInfo* const_iterator;

    const_iterator begin() const    { return m_modules; }
    const_iterator end() const      { return m_modules + m_count; }


private:

    ModuleInfo  m_modules[MODULE_MAX_ENTRIES];  // Modules
    int         m_count;                        // Module count
};


#endif
