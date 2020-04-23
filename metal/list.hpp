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


template<typename T>
class List
{
public:

    List() : m_head(nullptr), m_tail(nullptr), m_size(0) {};


    void push_back(T* node)
    {
        node->next = nullptr;

        if (m_head == nullptr)
        {
            m_head = m_tail = node;
        }
        else
        {
            m_tail->next = node;
            m_tail = node;
        }

        ++m_size;
    }


    T* pop_front()
    {
        T* node = m_head;

        if (node != nullptr)
        {
            m_head = node->next;
            node->next = nullptr;

            if (m_head == nullptr)
            {
                m_tail = nullptr;
            }

            --m_size;

            return node;
        }
        else
        {
            return nullptr;
        }
    }


    void remove(T* node)
    {
        T** pp = &m_head;

        while (*pp != nullptr)
        {
            if (*pp == node)
            {
                *pp = node->next;

                if (m_tail == node)
                {
                    m_tail = *pp;
                }

                node->next = nullptr;
                --m_size;
                break;
            }

            pp = &(*pp)->next;
        }
    }


    bool empty() const { return m_size == 0; }

    T* front() const { return m_head; }

    size_t size() const { return m_size; }


private:

    T* m_head;
    T* m_tail;
    size_t m_size;
};


#endif
