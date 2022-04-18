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

#include <expected>
#include <rainbow/boot.hpp>
#include <rainbow/uefi.hpp>
#include <vector>

class MemoryMap
{
public:
    MemoryMap(std::vector<efi::MemoryDescriptor>&& descriptors);

    // Print memory map to console
    void Print() const;

    // Tidy up the memory map, sorting and merging descriptors
    void TidyUp();

    // Allocate the specified number of memory pages.
    // TODO: we really would like std::optional<> here
    std::expected<PhysicalAddress, bool> AllocatePages(size_t pageCount);

    // Container interface
    using const_iterator = std::vector<efi::MemoryDescriptor>::const_iterator;

    const_iterator begin() const { return m_descriptors.begin(); }
    const_iterator end() const { return m_descriptors.end(); }
    size_t size() const { return m_descriptors.size(); }
    const efi::MemoryDescriptor* data() const { return m_descriptors.data(); }

    const efi::MemoryDescriptor& operator[](int index) const { return m_descriptors[index]; }

    // private:
    std::vector<efi::MemoryDescriptor> m_descriptors;
};
