/*
    Copyright (c) 2023, Thierry Tremblay
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

#include <cstddef>
#include <lai/core.h>
#include <metal/string_view.hpp>

class LaiState
{
public:
    LaiState() { lai_init_state(&m_state); }
    ~LaiState() { lai_finalize_state(&m_state); }

    LaiState(const LaiState&) = delete;
    LaiState& operator=(const LaiState&) = delete;

private:
    lai_state_t m_state;
};

class LaiNsChildIterator;

class LaiNsNode : public lai_nsnode_t
{
public:
    LaiNsNode(const LaiState&) = delete;
    LaiNsNode& operator=(const LaiState&) = delete;

    mtl::string_view GetName() const
    {
        size_t length = 4;

        while (length > 0 && name[length - 1] == '_')
            --length;

        return mtl::string_view{name, length};
    }

    // Iterate through children nodes
    using const_iterator = LaiNsChildIterator;

    const_iterator begin() const;
    const_iterator end() const;
};

// Ensure we can safely cast from lai_nsnode_t to LaiNsNode: we can't add members or methods.
static_assert(sizeof(LaiNsNode) == sizeof(lai_nsnode_t));

class LaiNsChildIterator
{
public:
    LaiNsChildIterator(const lai_nsnode_t* parent)
    {
        lai_initialize_ns_child_iterator(&m_iterator, const_cast<lai_nsnode_t*>(parent));
        ++*this;
    }

    LaiNsChildIterator(std::nullptr_t) : m_value{nullptr} {}

    const LaiNsNode* get() const { return m_value; }
    const LaiNsNode* operator->() const { return get(); }
    const LaiNsNode& operator*() const { return *get(); }

    LaiNsChildIterator& operator++()
    {
        m_value = static_cast<LaiNsNode*>(lai_ns_child_iterate(&m_iterator));
        return *this;
    }

    LaiNsChildIterator operator++(int)
    {
        LaiNsChildIterator result{*this};
        ++*this;
        return result;
    }

private:
    lai_ns_child_iterator m_iterator;
    LaiNsNode* m_value;
};

inline bool operator==(const LaiNsChildIterator& a, const LaiNsChildIterator& b)
{
    return a.get() == b.get();
}

inline LaiNsNode::const_iterator LaiNsNode::begin() const
{
    return LaiNsChildIterator{this};
}

inline LaiNsNode::const_iterator LaiNsNode::end() const
{
    return LaiNsChildIterator{nullptr};
}
