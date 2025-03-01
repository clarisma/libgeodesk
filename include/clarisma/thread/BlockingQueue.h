// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <clarisma/util/log.h>

namespace clarisma {

template <typename T>
class BlockingQueue
{
public:
    explicit BlockingQueue(int size) :
        size_(size),
        count_(0),
        front_(0),
        rear_(0)
    {
        assert(size > 0);
        queue_.resize(size);
    }

    void put(T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == size_)
        {
            notFull_.wait(lock, [this] { return count_ < size_; });
        }
        queue_[rear_] = std::move(item);
        rear_ = (rear_ + 1) % size_;
        count_++;
        notEmpty_.notify_one();
    }

    bool offer(T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == size_) return false;
        queue_[rear_] = std::move(item);
        rear_ = (rear_ + 1) % size_;
        count_++;
        notEmpty_.notify_one();
        return true;
    }

    T take()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == 0)
        {
            notEmpty_.wait(lock, [this] { return count_ > 0; });
        }
        T item = std::move(queue_[front_]);
        front_ = (front_ + 1) % size_;
        count_--;
        notFull_.notify_one();
        return item;
    }

    std::optional<T> poll()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == 0)
        {
            return std::nullopt;
        }
        T item = std::move(queue_[front_]);
        front_ = (front_ + 1) % size_;
        count_--;
        notFull_.notify_one();
        return item;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        count_ = 0;
        front_ = 0;
        rear_ = 0;
        notFull_.notify_all();
    }

private:
    std::vector<T> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_, notFull_;
    int front_;
    int rear_;
    int size_;
    int count_;
};

} // namespace clarisma
