// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef _RS_MEMORY_POOL_HH
#define _RS_MEMORY_POOL_HH

#include <queue>
#include <mutex>
#include <iostream>
#include <ipDeviceCommon/RsCommon.h>

#define POOL_SIZE 100                 //TODO:: to define the right value
#define MAX_FRAME_SIZE 1280 * 720 * 3 //TODO:: to define the right value
class MemoryPool
{

public:
    MemoryPool()
    {
        //alloc memory
        std::unique_lock<std::mutex> lk(m_mutex);
        for (int i = 0; i < POOL_SIZE; i++)
        {
            unsigned char *mem = new unsigned char[sizeof(RsFrameHeader) + MAX_FRAME_SIZE]; //TODO:to use OutPacketBuffer::maxSize;
            m_pool.push(mem);
        }
        lk.unlock();
    }

    MemoryPool(const MemoryPool &obj)
    {
        m_pool = obj.m_pool;
    }

    MemoryPool &operator=(const MemoryPool &&obj)
    {
        m_pool = obj.m_pool;
        return *this;
    }

    unsigned char *getNextMem()
    {
        unsigned char *mem = nullptr;
        std::unique_lock<std::mutex> lk(m_mutex);
        if (!m_pool.empty())
        {
            mem = m_pool.front();
            m_pool.pop();
        }
        else
        {
            std::cout << "pool is empty\n";
        }
        lk.unlock();
        //printf("memory_pool:  after getMem: pool size is: %d, mem is %p\n",pool.size(),mem);
        return mem;
    }

    void returnMem(unsigned char *t_mem)
    {
        if (t_mem != nullptr)
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_pool.push(t_mem);
            lk.unlock();
            //printf("memory_pool: after returnMem: pool size is: %d, mem is %p\n",pool.size(),mem);
        }
        else
        {
            std::cout << "returnMem:invalid mem\n";
        }
    }

    ~MemoryPool()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        while (!m_pool.empty())
        {
            unsigned char *mem = m_pool.front();
            //printf("memory_pool: ~memory_pool:not empty, mem is %p\n",mem);
            m_pool.pop();
            if (mem != nullptr)
            {
                delete mem;
            }
            else
            {
                std::cout << "~memory_pool:invalid mem\n";
            }
        }
        lk.unlock();
    }

private:
    std::queue<unsigned char *> m_pool;
    std::mutex m_mutex;
};

#endif
