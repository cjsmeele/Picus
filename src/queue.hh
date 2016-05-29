/**
 * \file
 * \brief     Fixed-length queue type.
 * \author    Chris Smeele
 * \copyright Copyright (c) 2016, Chris Smeele
 *
 * \page License
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <cstdint>
#include <cstdlib>

template<typename T, size_t size>
class Queue {
    T elems[size];
    T *begin;
    T *end;
    size_t length;
    T *head; ///< Current head (where we dequeue from).
    T *tail; ///< Current tail (where new items are enqueued).

    void incPtr(T **ptr) {
        if (++(*ptr) > this->end)
            *ptr = this->begin;
    }

public:
    void push(T elem) {
        if (length < size) {
            *tail = elem;
            incPtr(&tail);
            length++;
        }
    }

    T pop() {
        if (length) {
            T elem = *head;
            incPtr(&head);
            length--;
            return elem;
        } else {
            return T();
        }
    }

    T peek() {
        if (length)
            return *head;
        else
            return T();
    }

    size_t getLength() { return length; }

    Queue &operator+=(T elem) { push(elem); return *this; }
    T      operator--()       { return pop(); };
    T      operator*() const  { return peek(); };

    Queue() {
        begin  = &elems[0];
        end    = &elems[size-1];
        head   = tail = begin;
        length = 0;
    }

    ~Queue() = default;
};
