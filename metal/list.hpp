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

#ifndef _RAINBOW_METAL_LIST_HPP
#define _RAINBOW_METAL_LIST_HPP

#include <stddef.h>
#include "crt.hpp"


template<typename T>
class List
{
public:

    void push_back(T* node)
    {
        assert(node->prev == nullptr);
        assert(node->next == nullptr);

        if (m_tail)
        {
            m_tail->next = node;
            node->prev = m_tail;
            m_tail = node;
        }
        else
        {
            m_head = m_tail = node;
        }
    }


    T* pop_front()
    {
        if (m_head != m_tail)
        {
            T* node = m_head;
            m_head = node->next;
            m_head->prev = nullptr;
            node->next = nullptr;
            return node;
        }
        else if (m_head)
        {
            T* node = m_head;
            m_head = m_tail = nullptr;
            return node;
        }
        else
        {
            return nullptr;
        }
    }


    void remove(T* node)
    {
        if (node->prev)
        {
            node->prev->next = node->next;
        }
        else
        {
            m_head = node->next;
        }

        if (node->next)
        {
            node->next->prev = node->prev;
        }
        else
        {
            m_tail = node->prev;
        }

        node->prev = node->next = nullptr;
    }


    bool empty() const { return m_head == nullptr; }

    T* front() const { return m_head; }


private:

    T* m_head;
    T* m_tail;
};


#endif
