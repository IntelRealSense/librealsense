/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#include <stdlib.h>
#include <assert.h>

#include "rosbag/buffer.h"

//#include <ros/ros.h>

namespace rosbag {

Buffer::Buffer() : buffer_(NULL), capacity_(0), size_(0) { }

Buffer::~Buffer() {
    free(buffer_);
}

uint8_t* Buffer::getData()           { return buffer_;   }
uint32_t Buffer::getCapacity() const { return capacity_; }
uint32_t Buffer::getSize()     const { return size_;     }

void Buffer::setSize(uint32_t size) {
    size_ = size;
    ensureCapacity(size);
}

void Buffer::ensureCapacity(uint32_t capacity) {
    if (capacity <= capacity_)
        return;

    if (capacity_ == 0)
        capacity_ = capacity;
    else {
        while (capacity_ < capacity)
            capacity_ *= 2;
    }

    buffer_ = (uint8_t*) realloc(buffer_, capacity_);
    assert(buffer_);
}

} // namespace rosbag
