// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Poller"
#define LOG_NDEBUG 1    // controls LOGV only
#include "Log.h"
#include "Utils.h"
#include "Poller.h"
#include "TrackingCommon.h"


namespace perc 
{

    struct Poller::CheshireCat
    {
        std::unordered_map<Handle, Poller::event> mEvents;
        std::mutex mEventsGuard;

        std::mutex mAddRemoveMutex;
        std::vector<HANDLE> mHandles;
        HANDLE mAddRemoveDoneEvent;
    };

    const int Poller::READ_MASK = 0;
    const int Poller::NULL_MASK = 0;
    const int Poller::WRITE_MASK = 0;
    const int Poller::EXCEPT_MASK = 0;
    const int Poller::ALL_MASK = 0;


    Poller::Poller() : mData(new CheshireCat())
    {
        mData->mAddRemoveDoneEvent = CreateEvent(
            NULL,               // default security attributes
            TRUE,               // manual-reset event
            FALSE,              // initial state is nonsignaled
            NULL                // Unnamed event
        );
        ASSERT(mData->mAddRemoveDoneEvent != ILLEGAL_HANDLE);

        mData->mHandles.push_back(mData->mAddRemoveDoneEvent);
    }

    Poller::~Poller()
    {
        CloseHandle(mData->mAddRemoveDoneEvent);
    }

    bool Poller::add(const Poller::event &evt)
    {
        // Take Add remove/mutex
        std::lock_guard<std::mutex> guardAddRemove(mData->mAddRemoveMutex);

        // Notify poll thread to exit and release mEventsGuard
        SetEvent(mData->mAddRemoveDoneEvent);

        // Take mEventsGuard - wait for poll thread to release the mutex
        std::lock_guard<std::mutex> guardEvents(mData->mEventsGuard);

        ResetEvent(mData->mAddRemoveDoneEvent);

        //check if this handle already exists
        auto it = std::find(mData->mHandles.begin(), mData->mHandles.end(), evt.handle);

        // we are already at maximum events
        if (mData->mHandles.size() >= MAXIMUM_WAIT_OBJECTS && it == mData->mHandles.end())
            return false;

        if (it == mData->mHandles.end())
            mData->mHandles.push_back(evt.handle);

        // Add or modify event in events map
        mData->mEvents[evt.handle] = evt;

        return true;
    }

    bool Poller::remove(Handle handle)
    {
        // Take Add remove/mutex
        std::lock_guard<std::mutex> guardAddRemove(mData->mAddRemoveMutex);

        // Notify poll thread to exit and release mEventsGuard
        SetEvent(mData->mAddRemoveDoneEvent);

        // Take mEventsGuard - wait for poll thread to release the mutex
        std::lock_guard<std::mutex> guardEvents(mData->mEventsGuard);

        ResetEvent(mData->mAddRemoveDoneEvent);

        //check if this handle already exists
        auto it = std::find(mData->mHandles.begin(), mData->mHandles.end(), handle);

        if (it == mData->mHandles.end())
            return false;

        // Remove the handle from vector
        mData->mHandles.erase(it);

        // remove the evt from events map
        mData->mEvents.erase(handle);

        return true;
    }

    int Poller::poll(Poller::event &evt, unsigned long timeoutMs)
    {
        int timeoutAbs = (timeoutMs == INFINITE)? INFINITE : ns2ms(systemTime()) + (int)timeoutMs;

        while (true)
        {
            int exitReason;
            {
                // Take events guard, to wait safely on vector of handles
                std::lock_guard<std::mutex> guardEvents(mData->mEventsGuard);

                exitReason = WaitForMultipleObjects((DWORD)mData->mHandles.size(),
                    mData->mHandles.data(),
                    FALSE,
                    timeoutMs);

                int eventIndex = exitReason - WAIT_OBJECT_0;
                if ((exitReason < 0) || ((unsigned int)eventIndex >= mData->mHandles.size()))
                {
                    return 0;
                }
                else if (eventIndex != 0)  // zero is special case - it is our private event
                {
                    evt = mData->mEvents[mData->mHandles[eventIndex]];
                    return 1;
                }
            }         // here we release the events guard, so Add/remove function may continue

            // WAIT_OBJECT_0 means add/remove function called, lets wait for it completion by just waiting for the lock
            {
                std::lock_guard<std::mutex> guardAddRemove(mData->mAddRemoveMutex);

                // we are going to a second iteration - update timeout
                if (timeoutMs != INFINITE)
                {
                    auto nowMs = ns2ms(systemTime());
                    if (nowMs >= timeoutAbs)
                        timeoutMs = 0;
                    else
                        timeoutMs = timeoutAbs - nowMs;
                }
            }
        }
        return 0;
    }
} // namespace perc
