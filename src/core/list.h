/*   This file is part of fus.
 *
 *   fus is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   fus is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with fus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FUS_LIST_HPP
#define __FUS_LIST_HPP

#include <cstdlib>

#include "errors.h"

/**
 * \file list.h
 * \brief An intrusive doubly-linked list.
 * \remarks This file is largely based on the work of ArenaNet cofounder Patrick Wyatt. According
 *          to his claims, this mirrors the linked list used in Guild Wars 1. The code mirrors that
 *          found in the linked list implementation in CWE, whose author (eap) did a stint at ArenaNet.
 *          Seems, fitting, I think, to keep it all in the family. And yes, all of this text is here
 *          to give proper credit to the proper folks.
 */

/**
 * \biref Declares an intrusive list member.
 * Shortcut macro for declaring an intrusive list.
 * \param cls_name Class name of the list elements
 * \param link_name Name of the member field in the class to use for link usage \sa FUS_LIST_LINK
 */
#define FUS_LIST_DECL(cls_name, link_name) fus::list_declare<cls_name, offsetof(cls_name, link_name)>

/**
 * \brief Declares an intrusive list link field.
 * This shortcut macro should be used to declare a link field to be used for membership in an
 * intrusive list. Each list that the object is a member of should have its own link field.
 */
#define FUS_LIST_LINK(cls_name) fus::list_link<cls_name>

// =================================================================================

namespace fus
{
    template<class T>
    class list_link {
    public:
        ~list_link();
        list_link();

        list_link(const list_link& copy) = delete;
        list_link& operator=(const list_link& copy) = delete;

        bool linked() const;
        void unlink();

        T* prev();
        T* next();
        const T* prev() const;
        const T* next() const;

        // For use by list-type classes, not user code; the alternative is to friend list<T>
        list_link(size_t offset);
        void set_offset(size_t offset);
        list_link<T>* next_link();
        list_link<T>* prev_link();
        void insert_before(T* node, list_link<T>* nextLink);
        void insert_after(T* node, list_link<T>* prevLink);

    private:
        T *        m_nextNode; // pointer to the next >object<
        list_link<T> *  m_prevLink; // pointer to the previous >link field<
        void remove();
    };

    template<class T>
    list_link<T>::~list_link()
    {
        remove();
    }

    template<class T>
    list_link<T>::list_link()
    {
        // Mark this node as the end of the list, with no link offset
        m_nextNode = (T*)((size_t)this + 1 - 0);
        m_prevLink = this;
    }

    template<class T>
    list_link<T>::list_link(size_t offset)
    {
        // Mark this node as the end of the list, with the link offset set
        m_nextNode = (T*)((size_t)this + 1 - offset);
        m_prevLink = this;
    }

    template<class T>
    void list_link<T>::set_offset(size_t offset)
    {
        // Mark this node as the end of the list, with the link offset set
        m_nextNode = (T*)((size_t)this + 1 - offset);
        m_prevLink = this;
    }

    template<class T>
    list_link<T> * list_link<T>::next_link()
    {
        // Calculate the offset from a node pointer to a link structure
        size_t offset = (size_t)this - ((size_t)m_prevLink->m_nextNode & ~1);

        // Get the link field for the next node
        return (list_link<T> *) (((size_t)m_nextNode & ~1) + offset);
    }

    template<class T>
    void list_link<T>::remove()
    {
        next_link()->m_prevLink = m_prevLink;
        m_prevLink->m_nextNode = m_nextNode;
    }

    template<class T>
    void list_link<T>::insert_before(T * node, list_link<T> * nextLink)
    {
        remove();

        m_prevLink = nextLink->m_prevLink;
        m_nextNode = m_prevLink->m_nextNode;

        nextLink->m_prevLink->m_nextNode = node;
        nextLink->m_prevLink = this;
    }

    template<class T>
    void list_link<T>::insert_after(T * node, list_link<T> * prevLink)
    {
        remove();

        m_prevLink = prevLink;
        m_nextNode = prevLink->m_nextNode;

        prevLink->next_link()->m_prevLink = this;
        prevLink->m_nextNode = node;
    }

    template<class T>
    bool list_link<T>::linked() const
    {
        return m_prevLink != this;
    }

    template<class T>
    void list_link<T>::unlink()
    {
        remove();

        // Mark this node as the end of the list with no link offset
        m_nextNode = (T *)((size_t)this + 1);
        m_prevLink = this;
    }

    template<class T>
    list_link<T>* list_link<T>::prev_link()
    {
        return m_prevLink;
    }

    template<class T>
    T* list_link<T>::prev() {
        T* prevNode = m_prevLink->m_prevLink->m_nextNode;
        if ((size_t)prevNode & 1)
            return nullptr;
        return prevNode;
    }

    template<class T>
    const T* list_link<T>::prev() const
    {
        const T* prevNode = m_prevLink->m_prevLink->m_nextNode;
        if ((size_t)prevNode & 1)
            return nullptr;
        return prevNode;
    }

    template<class T>
    T* list_link<T>::next()
    {
        if ((size_t)m_nextNode & 1)
            return nullptr;
        return m_nextNode;
    }

    template<class T>
    const T* list_link<T>::next() const
    {
        if ((size_t)m_nextNode & 1)
            return nullptr;
        return m_nextNode;
    }

// =================================================================================

    template<class T>
    class list {
    public:
        ~list();
        list();

        list(const list& copy) = delete;
        list& operator=(const list& copy) = delete;

        bool empty() const;
        void clear();

        T* front();
        T* back();
        const T* front() const;
        const T* back() const;

        T* prev(T* node);
        T* next(T* node);
        const T* prev(const T* node) const;
        const T* next(const T* node) const;

        void push_front(T* node);
        void push_back(T* node);

        void insert_before(T* node, T* before);
        void insert_after(T* node, T*  after);

    private:
        list_link<T> m_link;
        size_t       m_offset;

        list(size_t offset);
        list_link<T>* get_link(const T * node) const;
        template<class _BaseT, size_t offset> friend class list_declare;
    };

// =================================================================================

    template<class T>
    list<T>::~list()
    {
        clear();
    }

    template<class T>
    list<T>::list()
        : m_link(), m_offset((size_t)-1)
    { }

    template<class T>
    list<T>::list(size_t offset)
        : m_link(offset), m_offset(offset)
    { }

    template<class T>
    bool list<T>::empty() const
    {
        return m_link.next() == nullptr;
    }

    template<class T>
    void list<T>::clear()
    {
        for (;;) {
            list_link<T> * link = m_link.prev_link();
            if (link == &m_link)
                break;
            link->unlink();
        }
    }

    template<class T>
    T* list<T>::front()
    {
        return m_link.next();
    }

    template<class T>
    T* list<T>::back()
    {
        return m_link.prev();
    }

    template<class T>
    const T* list<T>::front() const
    {
        return m_link.next();
    }

    template<class T>
    const T* list<T>::back() const
    {
        return m_link.prev();
    }

    template<class T>
    T* list<T>::prev(T* node)
    {
        return get_link(node)->prev();
    }

    template<class T>
    const T* list<T>::prev(const T* node) const
    {
        return get_link(node)->prev();
    }

    template<class T>
    T* list<T>::next(T* node)
    {
        return get_link(node)->next();
    }

    template<class T>
    const T* list<T>::next(const T* node) const
    {
        return get_link(node)->next();
    }

    template<class T>
    void list<T>::push_front(T* node)
    {
        insert_after(node, nullptr);
    }

    template<class T>
    void list<T>::push_back(T* node)
    {
        insert_before(node, nullptr);
    }

    template<class T>
    void list<T>::insert_before(T* node, T* before)
    {
        FUS_ASSERTD(!((size_t)node & 1));
        get_link(node)->insert_before(node,
                                      before ? get_link(before) : &m_link);
    }

    template<class T>
    void list<T>::insert_after(T* node, T* after)
    {
        FUS_ASSERTD(!((size_t)node & 1));
        get_link(node)->insert_after(node,
                                     after ? get_link(after) : &m_link);
    }

    template<class T>
    list_link<T>* list<T>::get_link(const T* node) const
    {
        FUS_ASSERTD(m_offset != (size_t)-1);
        return (list_link<T>*)((size_t)node + m_offset);
    }

// =================================================================================
    template<class _BaseT, size_t offset>
    class list_declare : public list<_BaseT>
    {
    public:
        list_declare() : list<_BaseT>(offset) { }
    };
};

//===================================
// MIT License
//
// Copyright (c) 2010 by Patrick Wyatt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//===================================
#endif
