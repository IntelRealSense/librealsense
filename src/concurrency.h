// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

const int QUEUE_MAX_SIZE = 10;
// Simplest implementation of a blocking concurrent queue for thread messaging
template<class T>
class single_consumer_queue
{
    std::deque<T> q;
    std::mutex mutex;
    std::condition_variable cv; // not empty signal
    unsigned int cap;
    bool accepting;

    // flush mechanism is required to abort wait on cv
    // when need to stop
    std::atomic<bool> need_to_flush;
    std::atomic<bool> was_flushed;
    std::condition_variable was_flushed_cv;
    std::mutex was_flushed_mutex;
public:
    explicit single_consumer_queue<T>(unsigned int cap = QUEUE_MAX_SIZE)
        : q(), mutex(), cv(), cap(cap), need_to_flush(false), was_flushed(false), accepting(true)
    {}

    void enqueue(T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (accepting)
        {
            q.push_back(std::move(item));
            if (q.size() > cap)
            {
                q.pop_front();
            }
        }
        lock.unlock();
        cv.notify_one();
    }

    bool dequeue(T* item ,unsigned int timeout_ms = 5000)
    {
        std::unique_lock<std::mutex> lock(mutex);
        accepting = true;
        was_flushed = false;
        const auto ready = [this]() { return (q.size() > 0) || need_to_flush; };
        if (!ready() && !cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready))
        {
            return false;
        }

        if (q.size() <= 0)
        {
            return false;
        }
        *item = std::move(q.front());
        q.pop_front();
        return true;
    }

    bool peek(T** item)
    {
        std::unique_lock<std::mutex> lock(mutex);

        if (q.size() <= 0)
        {
            return false;
        }
        *item = &q.front();
        return true;
    }

    bool try_dequeue(T* item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        accepting = true;
        if (q.size() > 0)
        {
            auto val = std::move(q.front());
            q.pop_front();
            *item = std::move(val);
            return true;
        }
        return false;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex);

        accepting = false;
        need_to_flush = true;

        while (q.size() > 0)
        {
            auto item = std::move(q.front());
            q.pop_front();
        }
        cv.notify_all();
    }

    void start()
    {
        std::unique_lock<std::mutex> lock(mutex);
        need_to_flush = false;
        accepting = true;
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return q.size();
    }
};


class dispatcher
{
public:
    class cancellable_timer
    {
    public:
        cancellable_timer(dispatcher* owner)
            : _owner(owner)
        {}

        bool try_sleep(int ms)
        {
            using namespace std::chrono;

            std::unique_lock<std::mutex> lock(_owner->_was_stopped_mutex);
            auto good = [&]() { return _owner->_was_stopped.load(); };
            return !(_owner->_was_stopped_cv.wait_for(lock, milliseconds(ms), good));
        }

    private:
        dispatcher* _owner;
    };

    dispatcher(unsigned int cap)
        : _queue(cap),
          _was_stopped(true),
          _was_flushed(false),
          _is_alive(true)
    {
        _thread = std::thread([&]()
        {
            while (_is_alive)
            {
                std::function<void(cancellable_timer)> item;

                if (_queue.dequeue(&item))
                {
                    cancellable_timer time(this);
                    item(time);
                }

#ifndef ANDROID
                std::unique_lock<std::mutex> lock(_was_flushed_mutex);
#endif
                _was_flushed = true;
                _was_flushed_cv.notify_all();
#ifndef ANDROID
                lock.unlock();
#endif
            }
        });
    }

    template<class T>
    void invoke(T item)
    {
        if (!_was_stopped)
        {
            _queue.enqueue(std::move(item));
        }
    }

    void start()
    {
        std::unique_lock<std::mutex> lock(_was_stopped_mutex);
        _was_stopped = false;

        _queue.start();
    }

    void stop()
    {
        {
            std::unique_lock<std::mutex> lock(_was_stopped_mutex);
            _was_stopped = true;
            _was_stopped_cv.notify_all();
        }

        _queue.clear();

        {
            std::unique_lock<std::mutex> lock(_was_flushed_mutex);
            _was_flushed = false;
        }

        std::unique_lock<std::mutex> lock_was_flushed(_was_flushed_mutex);
        _was_flushed_cv.wait_for(lock_was_flushed, std::chrono::hours(999999), [&]() { return _was_flushed.load(); });

        _queue.start();
    }

    ~dispatcher()
    {
        stop();
        _queue.clear();
        _is_alive = false;
        _thread.join();
    }

    bool flush()
    {
        std::mutex m;
        std::condition_variable cv;
        bool invoked = false;
        auto wait_sucess = std::make_shared<std::atomic_bool>(true);
        invoke([&, wait_sucess](cancellable_timer t)
        {
            ///TODO: use _queue to flush, and implement properly
            if (_was_stopped || !(*wait_sucess))
                return;

            {
                std::lock_guard<std::mutex> locker(m);
                invoked = true;
            }
            cv.notify_one();
        });
        std::unique_lock<std::mutex> locker(m);
        *wait_sucess = cv.wait_for(locker, std::chrono::seconds(10), [&]() { return invoked || _was_stopped; });
        return *wait_sucess;
    }
private:
    friend cancellable_timer;
    single_consumer_queue<std::function<void(cancellable_timer)>> _queue;
    std::thread _thread;

    std::atomic<bool> _was_stopped;
    std::condition_variable _was_stopped_cv;
    std::mutex _was_stopped_mutex;

    std::atomic<bool> _was_flushed;
    std::condition_variable _was_flushed_cv;
    std::mutex _was_flushed_mutex;

    std::atomic<bool> _is_alive;
};

template<class T = std::function<void(dispatcher::cancellable_timer)>>
class active_object
{
public:
    active_object(T operation)
        : _operation(std::move(operation)), _dispatcher(1), _stopped(true)
    {
    }

    void start()
    {
        _stopped = false;
        _dispatcher.start();

        do_loop();
    }

    void stop()
    {
        _stopped = true;
        _dispatcher.stop();
    }

    ~active_object()
    {
        stop();
    }
private:
    void do_loop()
    {
        _dispatcher.invoke([this](dispatcher::cancellable_timer ct)
        {
            _operation(ct);
            if (!_stopped)
            {
                do_loop();
            }
        });
    }

    T _operation;
    dispatcher _dispatcher;
    std::atomic<bool> _stopped;
};
