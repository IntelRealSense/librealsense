// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "infra/Poller"
#define LOG_NDEBUG 1    // controls LOGV only
#include "Log.h"
#include "Utils.h"
#include "Poller.h"
#include "TrackingCommon.h"
#include <unistd.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <vector>
#include <unordered_map>
#include <array>

namespace perc
{

    struct Poller::CheshireCat
    {
        std::unordered_map<Handle, struct kevent64_s> mEvents;
        std::mutex mEventsMutex;
        int mKQ;
    };

    const int Poller::READ_MASK = EVFILT_READ;
    const int Poller::NULL_MASK = 0;
    const int Poller::WRITE_MASK = EVFILT_WRITE;
    const int Poller::EXCEPT_MASK = EVFILT_EXCEPT;
    const int Poller::ALL_MASK = READ_MASK | WRITE_MASK | EXCEPT_MASK;

    Poller::Poller() : mData(new CheshireCat()) {
        mData->mKQ = ::kqueue();
        ASSERT(mData->mKQ != -1);
    }

    Poller::~Poller()
    {
        ::close(mData->mKQ);
    }

    bool Poller::add(const Poller::event &evt)
    {
        if (evt.handle == -1)
            return false;
        std::lock_guard<std::mutex> _(mData->mEventsMutex);
        struct kevent64_s e = {};
        e.ident = evt.handle;
        e.filter = evt.mask;
        e.flags = EV_ADD;
        e.udata = (intptr_t)evt.act;
        mData->mEvents[evt.handle] = e;
        return true;
    }

    bool Poller::remove(Handle handle)
    {
        if (handle == -1)
            return false;
        std::lock_guard<std::mutex> _(mData->mEventsMutex);
        auto evt = mData->mEvents.find(handle);
        if (evt == mData->mEvents.end())
            return false;
        mData->mEvents[handle].flags = EV_DELETE;
        return true;
    }

    int Poller::poll(Poller::event &evt, unsigned long timeoutMs)
    {
        std::vector<kevent64_s> changes;
        {
            std::lock_guard<std::mutex> guard(mData->mEventsMutex);
            changes.reserve(mData->mEvents.size());
            for (auto &e : mData->mEvents) {
                changes.push_back(e.second);
                if (e.second.flags == EV_DELETE)
                    mData->mEvents.erase(e.second.ident);
            }
        }
        std::array<kevent64_s,1> events;
        struct timespec timeout_, *timeout = timeoutMs == INFINITE ? nullptr : &timeout_;;
        timeout_.tv_sec  = timeoutMs / 1000;
        timeout_.tv_nsec = timeoutMs % 1000 * 1000000;
        int n = kevent64(mData->mKQ, changes.data(), changes.size(), events.data(), events.size(), 0, timeout);
        if (n == -1 && errno != EINTR)
            LOGE("kevent: epoll_wait error %d", errno);
        for (int i=0; i<n; i++)
            evt = Poller::event{ (int)events[i].ident, (unsigned long)events[i].filter, (void*)events[i].udata };
        return n;
    }
}
