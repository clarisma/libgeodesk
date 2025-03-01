// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

namespace clarisma {

template <typename T>
class LinkedStack 
{
public:
    LinkedStack() : first_(nullptr) {}
    LinkedStack(LinkedStack&& other) noexcept : first_(other.first_)
    {
        other.first_ = nullptr;
    }

    void clear()
    {
        first_ = nullptr;
    }

    // Copying is not allowed (only moving)
    LinkedStack(const LinkedStack&) = delete;
    LinkedStack& operator=(const LinkedStack&) = delete;

    bool isEmpty() const { return first_ == nullptr; }

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

    LinkedStack& operator=(LinkedStack&& other) noexcept
    {
        T* first = other.first_;
        other.first_ = nullptr;
        first_ = first;
        return *this;
    }

private:
    T* first_;
};

} // namespace clarisma
