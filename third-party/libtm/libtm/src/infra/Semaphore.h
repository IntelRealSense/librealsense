// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "Utils.h"
#include <memory>
/*
* Wrapper class for pthread semaphore implementation.
*/


namespace perc 
{

    class Semaphore
    {
    public:
        Semaphore(unsigned int initValue = 0);

        int get(nsecs_t timeoutNs = -1);
        int put();

        ~Semaphore();

    private:

        struct CheshireCat;

        std::unique_ptr<CheshireCat> mData;

        // = Prevent assignment and initialization.
        void operator= (const Semaphore &) = delete;
        Semaphore(const Semaphore &) = delete;

    };


} // namespace perc
