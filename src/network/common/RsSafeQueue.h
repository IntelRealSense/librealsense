// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>
#include <sstream>

class SafeQueue {
public:    
    SafeQueue() {};
    ~SafeQueue() {};

    void push(std::shared_ptr<uint8_t> e) {
        std::lock_guard<std::mutex> lck (m);
        return q.push(e);
    };

    std::shared_ptr<uint8_t> front() {
        std::lock_guard<std::mutex> lck (m);
        if (q.empty()) return NULL;
        return q.front();
    };

    void pop() {
        std::lock_guard<std::mutex> lck (m);
        if (q.empty()) return;
        return q.pop();
    };

    bool empty() {
        std::lock_guard<std::mutex> lck (m);
        return q.empty(); 
    };

private:
    std::queue<std::shared_ptr<uint8_t>> q;
    std::mutex m;
};
