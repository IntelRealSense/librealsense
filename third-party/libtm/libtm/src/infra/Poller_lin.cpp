/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#define LOG_TAG "infra/Poller"
#define LOG_NDEBUG 1    // controls LOGV only
#include "Log.h"
#include "Utils.h"
#include "Poller.h"
#include "TrackingCommon.h"
#include <unistd.h>
#include <sys/epoll.h>

namespace perc
{

    struct Poller::CheshireCat
    {
        std::unordered_map<Handle, Poller::event> mEvents;
        std::mutex mEventsGuard;
        int mEpoll;
    };

    const int Poller::READ_MASK = EPOLLIN | EPOLLRDNORM;
    const int Poller::NULL_MASK = 0;
    const int Poller::WRITE_MASK = EPOLLOUT | EPOLLWRNORM;
    const int Poller::EXCEPT_MASK = EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    const int Poller::ALL_MASK = READ_MASK | WRITE_MASK | EXCEPT_MASK;

    Poller::Poller() : mData(new CheshireCat()) {
        mData->mEpoll = ::epoll_create(1);
        ASSERT(mData->mEpoll != -1);
    }

    Poller::~Poller()
    {
        ::close(mData->mEpoll);
    }

    bool Poller::add(const Poller::event &evt)
    {
        int res = -1;
        if (evt.handle != -1) {
            struct epoll_event e;
            e.events = evt.mask;
            e.data.fd = evt.handle;
            // synchronized section
            std::lock_guard<std::mutex> guard(mData->mEventsGuard);
            if (mData->mEvents.count(evt.handle) == 0) {
                // adding new one
                res = ::epoll_ctl(mData->mEpoll, EPOLL_CTL_ADD, evt.handle, &e);
                if (!res)
                    mData->mEvents.emplace(evt.handle, evt);
            }
            else {
                // modifying old one
                res = ::epoll_ctl(mData->mEpoll, EPOLL_CTL_MOD, evt.handle, &e);
                if (!res)
                    mData->mEvents[evt.handle] = evt;
            }
        }
        return res == 0;
    }

    bool Poller::remove(Handle handle)
    {
        if (handle != -1) {
            //ASSERT(::pthread_equal(tid, ::pthread_self()));
            std::lock_guard<std::mutex> guard(mData->mEventsGuard);
            if (mData->mEvents.count(handle) > 0) {
                int res = ::epoll_ctl(mData->mEpoll, EPOLL_CTL_DEL, handle, 0);
                mData->mEvents.erase(handle);
                return res == 0;
            }
        }
        return false;
    }

    int Poller::poll(Poller::event &evt, unsigned long timeoutMs)
    {
        struct epoll_event e;
        int timeoutAbs = (int)timeoutMs == INFINITE ? INFINITE : ns2ms(systemTime()) + (int)timeoutMs;
    again:
        int n = ::epoll_wait(mData->mEpoll, &e, 1, (int)timeoutMs);
        if (n > 0) {
            std::lock_guard<std::mutex> guard(mData->mEventsGuard);
            LOGV("poll: e.data.fd %d", e.data.fd);
            if (mData->mEvents.count(e.data.fd) > 0) {
                evt = mData->mEvents[e.data.fd];
            }
            else {
                ::epoll_ctl(mData->mEpoll, EPOLL_CTL_DEL, e.data.fd, 0);
                int cur = ns2ms(systemTime());
                if ((int)timeoutMs != INFINITE) {
                    if (cur < (int)timeoutAbs)
                        timeoutMs = timeoutAbs - cur;
                    else
                        return 0;
                }
                goto again;
            }
        }
        else if (n == -1) {
            // TODO: add logs or clear errors
            LOGE("poll: epoll_wait error %d", errno);
        }
        // timeout
        return n;
    }

} // namespace perc
