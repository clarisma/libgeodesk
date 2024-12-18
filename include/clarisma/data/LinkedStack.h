// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

namespace clarisma {

template <typename T>
class LinkedStack 
{
public:
    LinkedStack() : first_(nullptr) {}

    void push(T* item)
    {
        item->setNext(first_);
        first_ = item;
    }

    T* pop()
    {
        T* first = first_;
        first_ = first->next();
        return first;
    }

    T* first() const { return first_; }

    T* takeAll()
    {
        T* first = first_;
        first_ = nullptr;
        return first;
    }

private:
    T* first_;
};

} // namespace clarisma
