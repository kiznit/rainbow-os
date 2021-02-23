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

#ifndef _RAINBOW_KERNEL_LIST_HPP
#define _RAINBOW_KERNEL_LIST_HPP

#include <cassert>


template<typename T>
class List
{
public:

    typedef T* iterator;
    typedef const T* const_iterator;


    List() : m_head(nullptr), m_tail(nullptr) {}

    T* pop_front()
    {
        T* p = m_head;

        if (p->m_next)
        {
            m_head = p->m_next;
            m_head->m_prev = nullptr;
            p->m_next = nullptr;
            return p;
        }
        else
        {
            m_head = m_tail = nullptr;
            return p;
        }
    }

    void push_back(T* p)
    {
        assert(p->m_next == nullptr);
        assert(p->m_prev == nullptr);

        if (m_tail)
        {
            m_tail->m_next = p;
            p->m_prev = m_tail;
            m_tail = p;
        }
        else
        {
            m_head = m_tail = p;
        }
    }

    T* remove(T* p)
    {
        // TODO: we are assuming "p" in is the list, how can we verify this?
        T* next = p->m_next;

        if (p->m_prev)
            p->m_prev->m_next = next;
        else
            m_head = next;

        if (next)
            next->m_prev = p->m_prev;
        else
            m_tail = p->m_prev;

        p->m_next = p->m_prev = nullptr;

        return next;
    }


    bool empty() const          { return m_head == nullptr; }

    T* front() const            { return m_head; }

private:

    T* m_head;
    T* m_tail;
};


#endif
