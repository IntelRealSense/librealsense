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

#include "rosbag/chunked_file.h"

#include <iostream>
#include <cstring>

#include <boost/format.hpp>

using std::string;
using boost::format;
using ros::Exception;

namespace rosbag {

UncompressedStream::UncompressedStream(ChunkedFile* file) : Stream(file) { }

CompressionType UncompressedStream::getCompressionType() const {
    return compression::Uncompressed;
}

void UncompressedStream::write(void* ptr, size_t size) {
    size_t result = fwrite(ptr, 1, size, getFilePointer());
    if (result != size)
        throw BagIOException((format("Error writing to file: writing %1% bytes, wrote %2% bytes") % size % result).str());

    advanceOffset(size);
}

void UncompressedStream::read(void* ptr, size_t size) {
    size_t nUnused = (size_t) getUnusedLength();
    char* unused = getUnused();

    if (nUnused > 0) {
        // We have unused data from the last compressed read
        if (nUnused == size) {
            // Copy the unused data into the buffer
            memcpy(ptr, unused, nUnused);

            clearUnused();
        }
        else if (nUnused < size) {
            // Copy the unused data into the buffer
            memcpy(ptr, unused, nUnused);

            // Still have data to read
            size -= nUnused;

            // Read the remaining data from the file
            int result = static_cast<int>(fread((char*) ptr + static_cast<int>(nUnused), 1, static_cast<int>(size), getFilePointer()));
            if ((size_t) result != size)
                throw BagIOException((format("Error reading from file + unused: wanted %1% bytes, read %2% bytes") % size % result).str());

            advanceOffset(size);

            clearUnused();
        }
        else {
            // nUnused_ > size
            memcpy(ptr, unused, size);

            setUnused(unused + size);
            setUnusedLength(static_cast<int>(nUnused - size));
        }
    }
    
    // No unused data - read from stream
    int result = static_cast<int>(fread( ptr, 1, static_cast<int>(size), getFilePointer()));
    if ((size_t) result != size)
        throw BagIOException((format("Error reading from file: wanted %1% bytes, read %2% bytes") % size % result).str());

    advanceOffset(size);
}

void UncompressedStream::decompress(uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len) {
    if (dest_len < source_len)
        throw BagException("dest_len not large enough");

    memcpy(dest, source, source_len);
}

} // namespace rosbag
