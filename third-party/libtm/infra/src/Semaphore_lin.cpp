/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include "Semaphore.h"
#include "Utils.h"
#include <semaphore.h>

namespace perc 
{

    struct Semaphore::CheshireCat
    {
        ::sem_t m_Semaphore;
    };

    int Semaphore::get(nsecs_t timeoutNs)
    {
        if (timeoutNs == -1)
            return ::sem_wait(&mData->m_Semaphore);
        if (timeoutNs == 0)
            return ::sem_trywait(&mData->m_Semaphore);
        nsecs_t  nsecsTime = systemTime() + timeoutNs;
        struct timespec absTimeout;
        absTimeout.tv_sec = ns2s(nsecsTime);
        absTimeout.tv_nsec = nsecsTime % 1000000000;
        return ::sem_timedwait(&mData->m_Semaphore, &absTimeout);
    }


    Semaphore::Semaphore(unsigned int initValue) : mData(new CheshireCat())
    {
        ::sem_init(&mData->m_Semaphore, 0, initValue);
    }

    Semaphore::~Semaphore()
    {
        ::sem_destroy(&mData->m_Semaphore);
    }

    int Semaphore::put()
    {
        return ::sem_post(&mData->m_Semaphore);
    }



} // namespace perc
