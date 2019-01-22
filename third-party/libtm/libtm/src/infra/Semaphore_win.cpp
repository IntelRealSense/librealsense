// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "Semaphore.h"
#include "Utils.h"
#include <assert.h>
#include <windows.h>

#define MAX_SEMAPHORE_COUNT 128

namespace perc {

    struct Semaphore::CheshireCat
    {
        HANDLE m_Semaphore;
    };

    int Semaphore::get(nsecs_t timeoutNs)
    {
        DWORD timeoutMs;

        timeoutMs = ns2ms(timeoutNs);

        auto ret = WaitForSingleObject(mData->m_Semaphore, timeoutMs);

        if (ret == WAIT_OBJECT_0)  // well this is useless , since WAIT_OBJECT_0 is equal to 0, but at least this is clear;
            return 0;

        return -1;
    }


    Semaphore::Semaphore(unsigned int initValue) : mData(new CheshireCat())
    {
        mData->m_Semaphore = CreateSemaphore(
            NULL,                   // default security attributes
            initValue,              // initial count
            MAX_SEMAPHORE_COUNT,    // maximum count
            NULL);

        assert(mData->m_Semaphore != NULL);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(mData->m_Semaphore);
    }

    int Semaphore::put() 
    {
        int ret = ReleaseSemaphore(
            mData->m_Semaphore, // handle to semaphore
            1,                  // increase count by one
            NULL);              // not interested in previous count

        return !ret;            // return zero on success; same as in linux
    }



} // namespace perc
