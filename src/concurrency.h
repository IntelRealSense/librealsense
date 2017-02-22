// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

// Simplest implementation of a blocking concurrent queue for thread messaging
template<class T>
class single_consumer_queue
{
    std::deque<T> q;
    std::mutex mutex;
    std::condition_variable cv;
    unsigned int cap;
    bool accepting = true;

public:
    explicit single_consumer_queue<T>(unsigned int cap)
        : q(), mutex(), cv(), cap(cap) {}

    void enqueue(T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (accepting) {
            q.push_back(std::move(item));
            if (q.size() > cap)
            {
                q.pop_front();
            }
        }
        lock.unlock();
        cv.notify_one();
    }

    T dequeue()
    {
        std::unique_lock<std::mutex> lock(mutex);
        accepting = true;
        const auto ready = [this]() { return !q.empty(); };
        if (!ready() && !cv.wait_for(lock, std::chrono::seconds(5), ready))
            throw std::runtime_error("Timeout waiting for queued items!");

        auto item = std::move(q.front());
        q.pop_front();
        return std::move(item);
    }

    bool try_dequeue(T* item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        accepting = true;
        if(q.size()>0)
        {
            auto val = std::move(q.front());
            q.pop_front();
            *item = std::move(val);
            return true;
        }
        return false;
    }

    void flush()
    {
        std::unique_lock<std::mutex> lock(mutex);
        accepting = false;
        while (q.size() > 0)
        {
            const auto ready = [this]() { return !q.empty(); };
            if (!ready() && !cv.wait_for(lock, std::chrono::seconds(5), ready))
                throw std::runtime_error("Timeout waiting for queued items!");

            auto item = std::move(q.front());
            q.pop_front();
        }
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return q.size();
    }
};
