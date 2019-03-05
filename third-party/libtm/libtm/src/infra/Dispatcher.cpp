// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Dispatcher"
#define LOG_NDEBUG 1    // controls LOGV only
#include "Log.h"
#include "Dispatcher.h"


namespace perc {
// ----------------------------------------------------------------------------
    Dispatcher::Dispatcher() : mThreadId(), m_Handlers(), m_HandlersGuard(), m_MessagesGuard(), m_Timers(), m_TimersGuard(), m_HoldersGuard(), m_Holders()
{
    bool ret = mPoller.add(Poller::event(mEvent.handle(), Poller::READ_MASK, this));
    if (ret != true)
    {
        ASSERT(ret);
    }
}

// ----------------------------------------------------------------------------
Dispatcher::~Dispatcher()
{
    processExit();
    {
        std::lock_guard<std::mutex> guard(m_MessagesGuard);
        for (int i = 0; i < PRIORITY_MAX; i++) {
            Holder *holder = (Holder *)m_Messages[i].GetHead();
            while (holder) {
                do {
                    m_Messages[i].RemoveHead();
                    delete holder;
                } while ((holder = (Holder *)m_Messages[i].GetHead()) != 0);
            }
        }
    }
    {
        std::lock_guard<std::mutex> guard(m_HandlersGuard);
        m_Handlers.clear();
    }
    {
        std::lock_guard<std::mutex> guard(m_TimersGuard);
        Holder *holder = (Holder *)m_Timers.GetHead();
        while (holder) {
            do {
                m_Timers.RemoveHead();
                delete holder;
            } while ((holder = (Holder *)m_Timers.GetHead()) != 0);
        }
    }
    mPoller.remove(mEvent.handle());
}

// ----------------------------------------------------------------------------
// DISPATCHER LOOP
//
// Returns:
//      >0 - the total number of timers, I/Oevents and messages that were dispatched in single call
//      0  - if the <timeout> elapsed without dispatching any handlers
//      -1 - if an error occurs
int Dispatcher::handleEvents(nsecs_t timeout)
{
    if (mExitPending) {
        LOGV("handleEvents(): processExit");
        processExit();
        return -1;
    }
    int ret = 0;
    mThreadId = std::this_thread::get_id();
    LOGV("handleEvents(): Poller::poll()");
    Poller::event event;
    int n = mPoller.poll(event, calculatePollTimeout(timeout));
    LOGV("handleEvents(): Poller::poll() ret %d", n);
    if (n > 0) {
        // process message queues: complete number of messages from message list that was signaled by event
        if (event.handle == mEvent.handle()) {
            mEvent.reset();
            LOGV("handleEvents(): processMessages");
            ret += processMessages();
        }
        else {
            // process file descriptor
            LOGV("handleEvents(): processEvent fd %d, revents %x", event.handle, event.mask);
            ret += processEvents(event);
        }
    }
    else if (n == -1) {
        LOGE("handleEvents(): Poller::poll() ret %d", n);
        return -1;
    }
    // process timers
    ret += processTimers();
    return ret;
}

// ----------------------------------------------------------------------------
void Dispatcher::endEventsLoop()
{
//    TRACE("");
    mExitPending = true;
    wakeup();
}

// ----------------------------------------------------------------------------
int Dispatcher::registerHandler(EventHandler *handler, Handle fd, unsigned long mask, void *act)
{
    if (mExitPending) return -1;
    ASSERT(handler);
    ASSERT(mThreadId == std::this_thread::get_id());
    int res = -1;
    HandlerHolder holder(handler, fd, mask, act);
    if (mPoller.add(holder.Event)) {
        std::lock_guard<std::mutex> guard(m_HandlersGuard);
        if (m_Handlers.count(fd) == 0) {
            // adding new one
            m_Handlers.emplace(fd, holder);
        }
        else {
            // modifying old one
            m_Handlers[fd] = holder;
        }
        res = 0;
    }
    return res;
}

// ----------------------------------------------------------------------------
int Dispatcher::registerHandler(EventHandler *handler)
{
    if (mExitPending) return -1;
    ASSERT(handler);
    std::lock_guard<std::mutex> guard(m_HoldersGuard);
    EmbeddedList::Iterator it(m_Holders);
    Holder *h = (Holder *)it.Current();
    // check if handler exists in a list
    while(h) {
        if (h->Handler == handler)
            return -1;
        h = (Holder *)it.Next();
    }
    Holder *holder = new Holder(handler);
    if (!holder) return -1;
    m_Holders.AddTail(holder);
    return 0;
}

// ----------------------------------------------------------------------------
void Dispatcher::removeHandle(Handle fd)
{
    ASSERT(mThreadId == std::this_thread::get_id());
    mPoller.remove(fd);
    std::lock_guard<std::mutex> guard(m_HandlersGuard);
    if (m_Handlers.count(fd) > 0) {
        m_Handlers.erase(fd);
    }
}

// ----------------------------------------------------------------------------
void Dispatcher::cancelTimer (uintptr_t timerId)
{
    ASSERT(timerId);
    ASSERT(mThreadId == std::this_thread::get_id());
    std::lock_guard<std::mutex> guard(m_TimersGuard);
    HolderTimer *holder = (HolderTimer *)timerId;
    // check if trying to cancel timer from <onTimeout> callback
    if (holder->Uptime) {
        m_Timers.Remove(holder);
        delete holder;
    }
}

// ----------------------------------------------------------------------------
int Dispatcher::removeHandler(EventHandler *handler, unsigned int mask)
{
    ASSERT(handler);
    ASSERT(mThreadId == std::this_thread::get_id());
    int removedHandlers = 0;

    if (mask & MESSAGES_MASK) {
        std::lock_guard<std::mutex> guard(m_MessagesGuard);
        for (int i = 0; i < PRIORITY_MAX; i++) {
            EmbeddedList::Iterator it(m_Messages[i]);
            Holder *holder = (Holder *)it.Current();
            while (holder) {
                Holder *curr = holder;
                holder = (Holder *)it.Next();
                if (curr->Handler == handler) {
                    m_Messages[i].Remove(curr);
                    delete curr;
                    removedHandlers++;
                }
            }
        }
    }
    if (mask & HANDLES_MASK) {
        std::lock_guard<std::mutex> guard(m_HandlersGuard);
        for (auto pair : m_Handlers) {
            const HandlerHolder &holder = pair.second;
            if (holder.Handler == handler) {
                Handle fd = pair.first;
                mPoller.remove(fd);
                m_Handlers.erase(fd);
                removedHandlers++;
            }
        }
    }
    if (mask & TIMERS_MASK) {
        std::lock_guard<std::mutex> guard(m_TimersGuard);
        EmbeddedList::Iterator it(m_Timers);
        Holder *holder = (Holder *)it.Current();
        while (holder) {
            Holder *curr = holder;
            holder = (Holder *)it.Next();
            if (curr->Handler == handler) {
                m_Timers.Remove(curr);
                delete curr;
                removedHandlers++;
            }
        }
    }
    if (mask & EXIT_MASK) {
        std::lock_guard<std::mutex> guard(m_HoldersGuard);
        EmbeddedList::Iterator it(m_Holders);
        Holder *holder = (Holder *)it.Current();
        while (holder) {
            Holder *curr = holder;
            holder = (Holder *)it.Next();
            if (curr->Handler == handler) {
                m_Holders.Remove(curr);
                delete curr;
                removedHandlers++;
                break;
            }
        }
    }

    return removedHandlers;
}

// ----------------------------------------------------------------------------
int Dispatcher::putMessage(Holder *holder, int priority)
{
    if (mExitPending) return -1;
    ASSERT(holder);
    if (priority >= PRIORITY_MAX)   priority = PRIORITY_MAX - 1;
    if (priority < 0)               priority = 0;
    std::lock_guard<std::mutex> guard(m_MessagesGuard);
    m_Messages[priority].AddTail(holder);
    // wake up running loop
    if (!wakeup()) {
        m_Messages[priority].Remove(holder);
        delete holder;
        return -1;
    }
    return 0;
}

// ----------------------------------------------------------------------------
uintptr_t Dispatcher::putTimer(EventHandler *handler, nsecs_t delay, Message *msg, int priority)
{
    // sanity check
    if ((mExitPending == true) || (msg == NULL))
    {
        return 0;
    }

    ASSERT(handler && delay != 0);
    // TODO: optimization: implement free list with local allocator
    HolderTimer *holder = new HolderTimer(handler, msg, delay);
    if (!holder)
    {
        delete msg;
        return 0;
    }
    std::lock_guard<std::mutex> guard(m_TimersGuard);
    EmbeddedList::Iterator it(m_Timers);
    HolderTimer *curr = (HolderTimer *)it.Current();
    if (!curr) {
        m_Timers.AddHead(holder);
    }
    else {
        do {
            if (holder->Uptime < curr->Uptime) {
                m_Timers.AddBefore(curr, holder);
                break;
            }
        } while ((curr = (HolderTimer *)it.Next()) != 0);
        if (!curr) {
            m_Timers.AddTail(holder);
        }
    }
    // reschedule timers by wake up running loop if was performed from another context
    if (mThreadId != std::this_thread::get_id())
        wakeup();
    return (uintptr_t)holder;
}

// ----------------------------------------------------------------------------
int Dispatcher::processMessages()
{
    // normalize wake-up events counter and real messages number before <onMessage> callback
    int cnt = 0;
    for (int i = 0; i < PRIORITY_MAX; i++) {
        cnt += m_Messages[i].Size();
    }
    int cntMsgs = cnt;
    while (cnt) {
        int priority = 0;
        for (int i = (PRIORITY_MAX - 1); i >= 0; i--) {
            if (m_Messages[i].Size()) {
                priority = i;
                break;
            }
        }
        m_MessagesGuard.lock();
        Holder *holder = (Holder *)m_Messages[priority].RemoveHead();
        m_MessagesGuard.unlock();
        if (holder) {
            holder->complete();
            delete holder;
        }
        else {
            break;
        }
        cnt--;
    }
    return cntMsgs;
}

// ----------------------------------------------------------------------------
int Dispatcher::processEvents(Poller::event &event)
{
    m_HandlersGuard.lock();
    auto it = m_Handlers.find(event.handle);
    if (it != m_Handlers.end()) {
        const HandlerHolder &holder = it->second;
        m_HandlersGuard.unlock();
        holder.Handler->onEvent(holder.Event.handle, event.mask, holder.Event.act);
        return 1;
    }
    else {
        mPoller.remove(event.handle);
        m_HandlersGuard.unlock();
    }
    return 0;
}

// ----------------------------------------------------------------------------
int Dispatcher::processTimers()
{
    int cnt = 0;
    HolderTimer* timer;
    m_TimersGuard.lock();
    timer = (HolderTimer *)m_Timers.GetHead();
    if (timer) {
        // work with head only, user can add new timers in a callback
        do {
            nsecs_t now = systemTime();
            if (timer->Uptime <= now) {
                m_Timers.RemoveHead ();
                m_TimersGuard.unlock();
                timer->complete();
                delete timer;
                m_TimersGuard.lock();
                cnt++;
            }
            else {
                break;
            }
        } while ((timer = (HolderTimer *)m_Timers.GetHead()) != 0);
    }
    m_TimersGuard.unlock();
    return cnt;
}

// ----------------------------------------------------------------------------
int Dispatcher::processExit()
{
    m_HoldersGuard.lock();
    Holder *holder;
    while ((holder = (Holder *)m_Holders.RemoveHead())) {
        m_HoldersGuard.unlock();
        holder->Handler->onExit();
        delete holder;
        m_HoldersGuard.lock();
    }
    m_HoldersGuard.unlock();
    return 0;
}

// ----------------------------------------------------------------------------
int Dispatcher::calculatePollTimeout(nsecs_t timeout)
{
    // calculate next wake-up time according to timers queue or user timeout
    std::lock_guard<std::mutex> guard(m_TimersGuard);
    HolderTimer *timer = (HolderTimer *)m_Timers.GetHead();
    if (timer) {
        int t1 = toMillisecondTimeoutDelay(systemTime(), timer->Uptime);
        int t2 = ns2ms(timeout);
        return ((unsigned long)t2 == INFINITE)? t1 : (t1 > t2 ? t2 : t1); 
    }

    // No timer in timer queue, returning timeout input in msec
    return ns2ms(timeout); 
}


// ----------------------------------------------------------------------------
} // namespace perc
