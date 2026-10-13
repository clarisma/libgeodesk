// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


#pragma once

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace clarisma {

/// Bounded, sequence-ordered, multi-producer/single-consumer queue.
/// Items must have unique, consecutive sequence numbers starting at zero.
/// Producers may submit items out of order. The consumer receives items
/// in sequence order.
///
/// Sequence numbers must not overflow uint64_t.
/// The queue must not be destroyed while threads are accessing it.
///
template<typename T>
class SequencedBlockingQueue
{
public:
    /// Creates a queue with the specified capacity.
    explicit SequencedBlockingQueue(size_t size) :
        queue_(std::make_unique<std::optional<T>[]>(size)),
        size_(size)
    {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
    }

    /// Inserts an item, blocking until its sequence enters the window.
    /// Throws std::invalid_argument for duplicate or obsolete sequences.
    void put(uint64_t sequence, T&& item)
    {
        std::unique_lock lock(mutex_);

        while (true)
        {
            if (sequence < next_)
            {
                throw std::invalid_argument("Obsolete sequence");
            }
            if (sequence - next_ < static_cast<std::uint64_t>(size_))
            {
                break;
            }
            notFull_.wait(lock);
        }

        auto& slot = queue_[sequence % size_];
        if (slot.has_value())
        {
            throw std::invalid_argument("Duplicate sequence");
        }

        slot.emplace(std::move(item));
        bool wakeConsumer = (sequence == next_);

        lock.unlock();

        if (wakeConsumer) notEmpty_.notify_one();
    }

    /// Attempts insertion without blocking.
    /// Returns false if the sequence is outside the current window.
    /// Throws std::invalid_argument for duplicate or obsolete sequences.
    bool offer(std::uint64_t sequence, T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (sequence < next_)
        {
            throw std::invalid_argument("Obsolete sequence");
        }
        if (sequence - next_ >= static_cast<std::uint64_t>(size_))
        {
            return false;
        }

        auto& slot = queue_[sequence % size_];
        if (slot.has_value())
        {
            throw std::invalid_argument("Duplicate sequence");
        }

        slot.emplace(std::move(item));
        bool wakeConsumer = (sequence == next_);

        lock.unlock();

        if (wakeConsumer) notEmpty_.notify_one();
        return true;
    }

    /// Removes the next item in sequence, blocking until available.
    /// Must be called by the single designated consumer.
    T take()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        notEmpty_.wait(lock, [this]
        {
            return queue_[next_ % size_].has_value();
        });

        auto& slot = queue_[next_ % size_];

        T item = std::move(*slot);
        slot.reset();
        ++next_;

        lock.unlock();
        notFull_.notify_all();

        return item;
    }

    /// Attempts to remove the next item without blocking.
    /// Returns std::nullopt if the expected sequence is unavailable.
    /// Must be called by the single designated consumer.
    std::optional<T> poll()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        auto& slot = queue_[next_ % size_];

        if (!slot.has_value())
        {
            return std::nullopt;
        }

        std::optional<T> item(std::move(*slot));
        slot.reset();
        ++next_;

        lock.unlock();
        notFull_.notify_all();

        return item;
    }

    /// Discards pending items and resets the expected sequence to zero.
    /// Producers must be stopped before calling this method.
    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        for (auto& slot : queue_)
        {
            slot.reset();
        }

        next_ = 0;

        lock.unlock();
        notFull_.notify_all();
    }

private:
    std::unique_ptr<std::optional<T>[]> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    const int size_;
    std::uint64_t next_ = 0;
};

} // namespace clarisma
