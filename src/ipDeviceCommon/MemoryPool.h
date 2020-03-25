// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <ipDeviceCommon/RsCommon.h>

#include <iostream>
#include <mutex>
#include <queue>

#include "NetdevLog.h"

#define POOL_SIZE 100

class MemoryPool
{

public:
    MemoryPool()
    {
        //alloc memory
        std::unique_lock<std::mutex> lk(m_mutex);
        for(int i = 0; i < POOL_SIZE; i++)
        {
            unsigned char *mem = new unsigned char[sizeof(RsFrameHeader) + MAX_FRAME_SIZE]; //TODO:to use OutPacketBuffer::maxSize;
            m_pool.push(mem);
        }
        lk.unlock();
    }

    MemoryPool(const MemoryPool& obj)
    {
        m_pool = obj.m_pool;
    }

    MemoryPool& operator=(const MemoryPool&& obj)
    {
        m_pool = obj.m_pool;
        return *this;
    }

    unsigned char* getNextMem()
    {
        unsigned char* mem = nullptr;
        std::unique_lock<std::mutex> lk(m_mutex);
        if(!m_pool.empty())
        {
            mem = m_pool.front();
            m_pool.pop();
        }
        else
        {
            ERR << "getNextMem: pool is empty";
        }
        lk.unlock();
        return mem;
    }

    void returnMem(unsigned char* t_mem)
    {
        if(t_mem != nullptr)
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_pool.push(t_mem);
            lk.unlock();
        }
        else
        {
            ERR << "returnMem: invalid address";
        }
    }

    ~MemoryPool()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        while(!m_pool.empty())
        {
            unsigned char* mem = m_pool.front();
            m_pool.pop();
            if(mem != nullptr)
            {
                delete mem;
            }
            else
            {
                ERR << "~memory_pool: invalid address";
            }
        }
        lk.unlock();
    }

private:
    std::queue<unsigned char*> m_pool;
    std::mutex m_mutex;
};
