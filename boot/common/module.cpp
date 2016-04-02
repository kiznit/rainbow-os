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

#include "module.hpp"
#include <stdio.h>
#include <string.h>

Modules::Modules()
{
    m_count = 0;
}



void Modules::AddModule(const char* name, physaddr_t start, physaddr_t end)
{
    // Ignore invalid entries (including zero-sized ones)
    if (start >= end)
        return;

    // If the table is full, we can't add more entries
    if (m_count == MODULE_MAX_ENTRIES)
        return;

    // Insert this new entry
    ModuleInfo* module = &m_modules[m_count];
    module->start = start;
    module->end = end;
    strncpy(module->name, name, MODULE_MAX_NAME_LENGTH-1);
    module->name[MODULE_MAX_NAME_LENGTH-1] = '\0';
    ++m_count;
}



void Modules::Print()
{
    printf("Modules:\n");

    for (int i = 0; i != m_count; ++i)
    {
        const ModuleInfo& module = m_modules[i];

        printf("    %016llx - %016llx : %s\n", module.start, module.end, module.name);
    }
}
