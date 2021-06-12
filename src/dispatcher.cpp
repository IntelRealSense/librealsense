// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "concurrency.h"
#include "types.h"
#include "../common/utilities/time/waiting-on.h"


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
                catch( const std::exception & e )
                {
                    LOG_ERROR( "Dispatcher [" << this << "] exception caught: " << e.what() );
                }
                catch( ... )
                {
                    LOG_ERROR( "Dispatcher [" << this << "] unknown exception caught!" );
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


// Return when all current items in the queue are finished (within a timeout).
// If additional items are added while we're waiting, those will not be waited on!
// Returns false if a timeout occurred before we were done
//
bool dispatcher::flush()
{
    if( _was_stopped )
        return true;  // Nothing to do - so success (no timeout)

#if 1
    waiting_on< bool > invoked( false );
    invoke( [invoked = invoked.in_thread()]( cancellable_timer ) {
        invoked.signal( true );
    } );
    invoked.wait_until( std::chrono::seconds( 10 ), [&]() {
        return invoked || _was_stopped;
    } );
    return invoked;
#else

    // We want to watch out for a timeout -- in which case this function will exit but the invoked
    // block is still not dispatched so alive! I.e., we cannot access m/cv/invoked then!
    struct wait_state_t
    {
        bool invoked = false;
        std::condition_variable cv;
    };
    auto wait_state = std::make_shared< wait_state_t >();

    // Add a function to the dispatcher that will set a flag and notify us when we get to it
    invoke( [still_waiting = std::weak_ptr< wait_state_t >( wait_state )]( cancellable_timer t ) {
        if( auto state = still_waiting.lock() )
        {
            state->invoked = true;
            state->cv.notify_one();
        }
    } );

    // Wait until 'invoked'
    std::mutex m;
    std::unique_lock< std::mutex > locker( m );
    wait_state->cv.wait_for( locker, std::chrono::seconds( 10 ), [&]() {
        return wait_state->invoked || _was_stopped;
    } );

    // If a timeout occurred: invoked will be false, _still_waiting will go out of scope and our
    // function, when invoked, would not try to reference any of the locals here
    return wait_state->invoked;
#endif
}
