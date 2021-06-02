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
    std::deque<T> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _deq_cv; // not empty signal
    std::condition_variable _enq_cv; // not full signal

    unsigned int const _cap;
    bool _accepting;

    std::function<void(T const &)> const _on_drop_callback;

public:
    explicit single_consumer_queue< T >( unsigned int cap = QUEUE_MAX_SIZE,
                                         std::function< void( T const & ) > on_drop_callback = nullptr )
        : _queue()
        , _mutex()
        , _deq_cv()
        , _enq_cv()
        , _cap( cap )
        , _accepting( true )
        , _on_drop_callback( on_drop_callback )
    {
    }

    // Enqueue an item onto the queue.
    // If the queue grows beyond capacity, the front will be removed, losing whatever was there!
    void enqueue(T&& item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if( ! _accepting )
        {
            if( _on_drop_callback )
                _on_drop_callback( item );
            return;
        }

        _queue.push_back(std::move(item));

        if( _queue.size() > _cap )
        {
            if( _on_drop_callback )
                _on_drop_callback( _queue.front() );
            _queue.pop_front();
        }

        lock.unlock();

        // We pushed something -- let others know there's something to dequeue
        _deq_cv.notify_one();
    }


    // Enqueue an item, but wait for room if there isn't any
    // Returns true if the enqueue succeeded
    bool blocking_enqueue(T&& item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if( ! _accepting )
        {
            if( _on_drop_callback )
                _on_drop_callback( item );
            return false;
        }

        if( _queue.size() >= _cap )
        {
            _enq_cv.wait( lock );
            if( ! _accepting )
            {
                // We shouldn't be adding anything to the queue when we're stopping
                if( _on_drop_callback )
                    _on_drop_callback( item );
                return false;
            }
        }

        _queue.push_back(std::move(item));
        lock.unlock();

        // We pushed something -- let another know there's something to dequeue
        _deq_cv.notify_one();

        return true;
    }


    // Remove one item; if unavailable, wait for it
    // Return true if an item was removed -- otherwise, false
    bool dequeue( T * item, unsigned int timeout_ms )
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if( _queue.empty() )
        {
            _deq_cv.wait_for( lock, std::chrono::milliseconds( timeout_ms ) );
            if( _queue.empty() )
                return false;
        }

        *item = std::move(_queue.front());
        _queue.pop_front();

        // We've made room -- let whoever is waiting for room know about it
        _enq_cv.notify_one();

        return true;
    }

    // Remove one item if available; do not wait for one
    // Return true if an item was removed -- otherwise, false
    bool try_dequeue(T* item)
    {
        std::lock_guard< std::mutex > lock( _mutex );
        if( _queue.empty() )
            return false;

        *item = std::move(_queue.front());
        _queue.pop_front();

        // We've made room -- let whoever is waiting for room know about it
        _enq_cv.notify_one();

        return true;
    }

    bool peek(T** item)
    {
        std::lock_guard< std::mutex > lock( _mutex );

        if (_queue.empty())
            return false;

        *item = &_queue.front();
        return true;
    }

    void stop()
    {
        std::lock_guard< std::mutex > lock( _mutex );

        // We no longer accept any more items!
        _accepting = false;

        _clear();
    }

    void clear()
    {
        std::lock_guard< std::mutex > lock( _mutex );
        _clear();
    }

protected:
	void _clear()
	{
        // Empty out the queue (TODO: why no just _queue.clear()?)
        while( _queue.size() > 0 )
        {
            auto item = std::move( _queue.front() );
            _queue.pop_front();
        }

        // Wake up anyone who is waiting for room to enqueue, or waiting for something to dequeue -- there's nothing now
        _enq_cv.notify_all();
        _deq_cv.notify_all();
    }

public:
    void start()
    {
        std::lock_guard< std::mutex > lock( _mutex );
        _accepting = true;
    }

    size_t size() const
    {
        std::lock_guard< std::mutex > lock( _mutex );
        return _queue.size();
    }

    bool empty() const { return ! size(); }
};

template<class T>
class single_consumer_frame_queue
{
    single_consumer_queue<T> _queue;

public:
    single_consumer_frame_queue<T>(unsigned int cap = QUEUE_MAX_SIZE) : _queue(cap) {}

    void enqueue(T&& item)
    {
        if (item.is_blocking())
            _queue.blocking_enqueue(std::move(item));
        else
            _queue.enqueue(std::move(item));
    }

    bool dequeue(T* item, unsigned int timeout_ms)
    {
        return _queue.dequeue(item, timeout_ms);
    }

    bool peek(T** item)
    {
        return _queue.peek(item);
    }

    bool try_dequeue(T* item)
    {
        return _queue.try_dequeue(item);
    }

    void clear()
    {
        _queue.clear();
    }

    void stop()
    {
        _queue.stop();
    }

    void start()
    {
        _queue.start();
    }

    size_t size() const
    {
        return _queue.size();
    }

    bool empty() const
    {
        return _queue.empty();
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

        bool try_sleep(std::chrono::milliseconds::rep ms)
        {
            using namespace std::chrono;

            std::unique_lock<std::mutex> lock(_owner->_was_stopped_mutex);
            auto good = [&]() { return _owner->_was_stopped.load(); };
            return !(_owner->_was_stopped_cv.wait_for(lock, milliseconds(ms), good));
        }

    private:
        dispatcher* _owner;
    };
    typedef std::function<void(cancellable_timer const &)> action;
    dispatcher(unsigned int cap, std::function <void(action)> on_drop_callback = nullptr)
        : _queue(cap, on_drop_callback),
          _was_stopped(true),
          _was_flushed(false),
          _is_alive(true)
    {
        _thread = std::thread([&]()
        {
            int timeout_ms = 5000;
            while (_is_alive)
            {
                std::function<void(cancellable_timer)> item;

                if (_queue.dequeue(&item, timeout_ms))
                {
                    cancellable_timer time(this);

                    try
                    {
                        item(time);
                    }
                    catch(...){}
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
    void invoke(T item, bool is_blocking = false)
    {
        if (!_was_stopped)
        {
            if(is_blocking)
                _queue.blocking_enqueue(std::move(item));
            else
                _queue.enqueue(std::move(item));
        }
    }

    template<class T>
    void invoke_and_wait(T item, std::function<bool()> exit_condition, bool is_blocking = false)
    {
        bool done = false;

        //action
        auto func = std::move(item);
        invoke([&, func](dispatcher::cancellable_timer c)
        {
            std::lock_guard<std::mutex> lk(_blocking_invoke_mutex);
            func(c);

            done = true;
            _blocking_invoke_cv.notify_one();
        }, is_blocking);

        //wait
        std::unique_lock<std::mutex> lk(_blocking_invoke_mutex);
        _blocking_invoke_cv.wait(lk, [&](){ return done || exit_condition(); });
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

            if (_was_stopped.load()) return;

            _was_stopped = true;
            _was_stopped_cv.notify_all();
        }

        _queue.stop();

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
        _is_alive = false;

        if( _thread.joinable() )
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

    bool empty()
    {
        return _queue.size() == 0;
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

    std::condition_variable _blocking_invoke_cv;
    std::mutex _blocking_invoke_mutex;

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
        if (!_stopped.load()) {
            _stopped = true;
            _dispatcher.stop();
        }
    }

    ~active_object()
    {
        stop();
    }

    bool is_active() const
    {
        return !_stopped;
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

class watchdog
{
public:
    watchdog(std::function<void()> operation, uint64_t timeout_ms) :
            _timeout_ms(timeout_ms), _operation(std::move(operation))
    {
        _watcher = std::make_shared<active_object<>>([this](dispatcher::cancellable_timer cancellable_timer)
        {
            if(cancellable_timer.try_sleep(_timeout_ms))
            {
                if(!_kicked)
                    _operation();
                std::lock_guard<std::mutex> lk(_m);
                _kicked = false;
            }
        });
    }

    ~watchdog()
    {
        if(_running)
            stop();
    }

    void start() { std::lock_guard<std::mutex> lk(_m); _watcher->start(); _running = true; }
    void stop() { { std::lock_guard<std::mutex> lk(_m); _running = false; } _watcher->stop(); }
    bool running() { std::lock_guard<std::mutex> lk(_m); return _running; }
    void set_timeout(uint64_t timeout_ms) { std::lock_guard<std::mutex> lk(_m); _timeout_ms = timeout_ms; }
    void kick() { std::lock_guard<std::mutex> lk(_m); _kicked = true; }

private:
    std::mutex _m;
    uint64_t _timeout_ms;
    bool _kicked = false;
    bool _running = false;
    std::function<void()> _operation;
    std::shared_ptr<active_object<>> _watcher;
};
