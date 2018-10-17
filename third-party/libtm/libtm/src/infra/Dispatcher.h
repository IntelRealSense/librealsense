// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "Utils.h"
#include "Event.h"
#include "Fence.h"
#include "EmbeddedList.h"
#include "EventHandler.h"
#include "Poller.h"

namespace perc {
// ----------------------------------------------------------------------------
///
/// @class  Dispatcher
///
/// @brief  Provides a event demultiplexing mechanism for messages, timers
///         and file descriptors events.
///
// ----------------------------------------------------------------------------
class Dispatcher
{
public:
    Dispatcher();
    ~Dispatcher();

    // Message notification mechanism
    enum
    {
        PRIORITY_IDLE = 0,  // Lowest priority
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_MAX,
    };

    // = Post/Send message
    //
    // User can call to this function from any running context.
    // Call to these functions will trigger <EventHandler::onMessage> callback.
    // Post message to <client> via the Dispatcher's notification mechanism, don't wait for execution.
    // This method will internally copy message (using copy constructor) and free after dispatching.
    template <class T>
    int postMessage(EventHandler *, const T&, int priority = Dispatcher::PRIORITY_IDLE);

    // Send message to <client> via the Dispatcher's notification mechanism and blocking wait for execution.
    // User SHOULD call to this function from different running context not <Dispatcher> one.
    // This function waits to the message processing by client and returns after <client> returns
    // from <EventHandler::onMessage> callback.
    // <Message> parameter is transferred by reference to callback - user may indicate return parameters if needed.
    // Don't call to this function in the same running context.
    template <class T>
    int sendMessage(EventHandler *, const T&, int priority = Dispatcher::PRIORITY_IDLE);

    // = Register/remove handler for I/O and OS events.
    //
    // A handler can be associated with multiple handles.
    // A handle cannot be associated with multiple handlers.
    // User can call to this function from any running context.
    // Call to these functions will trigger <EventHandler::onEvent> callback,
    // according to <Poller::mask> parameter
    int registerHandler(EventHandler *, Handle fd, unsigned long mask = Poller::READ_MASK, void *act = 0);

    // Register handler for Dispatcher exit processing, <EventHandler::onExit> callback should be called 
    // during Disptcher deactivation.
    int registerHandler(EventHandler *);

    // Remove handle from dispatcher and it's associated handler.
    // User SHOULD call these functions from dispatcher context only (from <EventHandler> callbacks)!!!
    void removeHandle(Handle fd);

    // Remove all messages (and release them), handles and timers (cancel them) associated with this handler
    // according to mask parameters.
    //
    // User SHOULD call these functions from dispatcher context only (from <EventHandler> callbacks)!!!
    enum
    {
        MESSAGES_MASK = (1<<0),
        HANDLES_MASK = (1<<1),
        TIMERS_MASK = (1<<2),
        EXIT_MASK = (1<<3),
        ALL_MASK = MESSAGES_MASK | HANDLES_MASK | TIMERS_MASK | EXIT_MASK,
    };
    int removeHandler(EventHandler *, unsigned int mask = Dispatcher::ALL_MASK);

    // = Timers.
    //
    // @params: delayMs     Time interval in nanoseconds after which the timer will expire.
    // @return  <timerId> or 0 in case of error.
    template <class T>
    uintptr_t scheduleTimer (EventHandler *, nsecs_t delay, const T&, int priority = Dispatcher::PRIORITY_IDLE);

    // Cancel timer associated with this <timerId>.
    // User SHOULD call these functions from dispatcher context only (from <EventHandler> callbacks)!!!
    void cancelTimer (uintptr_t timerId);

    // Main dispatch function
    //
    // Returns:
    //      >0 - the total number of timers, I/Oevents and messages that were dispatched in single call
    //      0  - if the <timeout> elapsed without dispatching any handlers
    //      -1 - if an error occurs
    int handleEvents(nsecs_t timeout = INFINITE);
    void endEventsLoop();

protected:

    // holder of messages base class
    class Holder : public EmbeddedListElement
    {
    public:
        Holder(EventHandler *h) : Handler(h) {}
        virtual ~Holder() {}
        virtual void complete() {}
        EventHandler *Handler;
    };

private:
    std::thread::id mThreadId;
    Event mEvent;
    Poller mPoller;
    bool mExitPending = false;

    int putMessage(Holder *, int priority);
    uintptr_t putTimer (EventHandler *, nsecs_t delay, Message *, int priority);
    int processMessages();
    int processEvents(Poller::event &);
    int processTimers();
    int processExit();

    /**
    * @brief This function calculates the next wake-up time according to timers queue or user timeout
    *
    * @param[in] timeout - poll timeout in nsec.
    * @return Next wake up time in msec.
    */
    int calculatePollTimeout(nsecs_t);
    bool wakeup() { return mEvent.signal(); }

    class HolderPost : public Holder
    {
    public:
        HolderPost(EventHandler *h, Message *m) : Holder(h), Msg(m) {}
        virtual ~HolderPost() { delete Msg; }
        virtual void complete() { Handler->onMessage(*Msg); }
        Message *Msg;

        /* Disallow these operations */
        HolderPost &operator=(const HolderPost &);
        HolderPost(const HolderPost &);
    }; 

    class HolderSend : public Holder
    {
    public:
        HolderSend(EventHandler *h, const Message &m, Fence &w) :
            Holder(h), Msg(m), Waiter(w) {}
        virtual ~HolderSend() { Waiter.notify(); }
        virtual void complete() { Handler->onMessage(Msg); }
        const Message &Msg;
        Fence &Waiter;  // sync wait lock handle
    };
    EmbeddedList m_Messages[PRIORITY_MAX];
    std::mutex m_MessagesGuard;

    // I/O HANDLERS
    struct HandlerHolder
    {
        EventHandler *Handler;
        Poller::event Event;
        HandlerHolder() : Handler(NULL) {}
        HandlerHolder(EventHandler *handler, Handle fd, unsigned long mask, void *act) :
            Handler(handler), Event(fd, mask, act) {}

    };
    std::unordered_map<Handle, HandlerHolder> m_Handlers;
    std::mutex m_HandlersGuard;

    // TIMERS
    class HolderTimer : public HolderPost
    {
    public:
        HolderTimer(EventHandler *h, Message *m, nsecs_t d) : HolderPost(h, m)
        { Uptime = systemTime() + d; }
        virtual void complete() { Uptime = 0; Handler->onTimeout((uintptr_t)this, *Msg); }
        nsecs_t Uptime;
    };
    EmbeddedList m_Timers;
    std::mutex m_TimersGuard;

    // HANDLERS 
    EmbeddedList m_Holders;
    std::mutex m_HoldersGuard;

    // Disallow these operations.
    Dispatcher &operator=(const Dispatcher &);
    Dispatcher(const Dispatcher &);
};


// ----------------------------------------------------------------------------
// Template functions MUST be implemented in header file with minimal code size
template <class T>
int Dispatcher::postMessage(EventHandler *handler, const T &msg, int priority)
{
    T::IS_DERIVED_FROM_Message;
    if (!handler) return -1;

    T *m = new T(msg); // use copy constructor to create message clone
    if (!m) return -ENOMEM;

    Holder *holder = new HolderPost(handler, m);
    if (!holder)
    {
        delete m;
        return -ENOMEM;
    }
    return putMessage(holder, priority);
}

// ----------------------------------------------------------------------------
template <class T>
int Dispatcher::sendMessage(EventHandler *handler, const T &msg, int priority)
{
    T::IS_DERIVED_FROM_Message;
    ASSERT(mThreadId != std::this_thread::get_id());
    // use message reference
    Fence f;
    Holder *holder = new HolderSend(handler, msg, f);
    if (!holder )
        return -ENOMEM;

    if (putMessage(holder, priority) < 0)
        return -1;

    // wake for message completion or reset by dispatcher
    f.wait();
    return 0;
}

// ----------------------------------------------------------------------------
template <class T>
uintptr_t Dispatcher::scheduleTimer (EventHandler *handler, nsecs_t delay, const T &msg, int priority)
{
    T::IS_DERIVED_FROM_Message;
    return putTimer(handler, delay, new T(msg), priority);
}


// ----------------------------------------------------------------------------
} // namespace perc
