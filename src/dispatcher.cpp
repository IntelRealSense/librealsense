// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "concurrency.h"


dispatcher::dispatcher( unsigned int cap, std::function< void( action ) > on_drop_callback )
    : _queue( cap, on_drop_callback )
    , _was_stopped( true )
    , _is_alive( true )
{
    // We keep a running thread that takes stuff off our queue and dispatches them
    _thread = std::thread([&]()
    {
        int timeout_ms = 5000;
        while( _is_alive )
        {
            std::function< void( cancellable_timer ) > item;

            if( _queue.dequeue( &item, timeout_ms ) )
            {
                cancellable_timer time(this);

                try
                {
                    // While we're dispatching the item, we cannot stop!
                    std::lock_guard< std::mutex > lock( _dispatch_mutex );
                    item( time );
                }
                catch( ... )
                {
                }
            }
        }
    });
}


dispatcher::~dispatcher()
{
    // Don't get into any more dispatches
    _is_alive = false;

    // Stop whatever's in-progress, if any
    stop();

    // Wait until our worker thread quits
    if( _thread.joinable() )
        _thread.join();
}


void dispatcher::start()
{
    std::lock_guard< std::mutex > lock( _was_stopped_mutex );
    _was_stopped = false;

    _queue.start();
}


void dispatcher::stop()
{
    // With the following commented-out if, we have issues!
    // It seems stop is called multiple times and the queues are somehow waiting on something after
    // the first time. If we return, those queues aren't woken! If we continue, the only effect will
    // be to notify_all and we get good behavior...
    // 
    //if( _was_stopped )
    //    return;

    // First things first: don't accept any more incoming stuff, and get rid of anything
    // pending
    _queue.stop();

    // Signal we've stopped so any sleeping dispatched will wake up immediately
    {
        std::lock_guard< std::mutex > lock( _was_stopped_mutex );
        _was_stopped = true;
        _was_stopped_cv.notify_all();
    }

    // Wait until any dispatched is done...
    {
        std::lock_guard< std::mutex > lock( _dispatch_mutex );
        assert( _queue.empty() );
    }
}


// Return when all items in the queue are finished (within a timeout).
// If additional items are added while we're waiting, those will not be waited on!
//
bool dispatcher::flush()
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
