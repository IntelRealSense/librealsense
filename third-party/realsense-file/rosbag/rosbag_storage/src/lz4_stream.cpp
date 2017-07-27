/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, Ben Charrow
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
#include "console_bridge/console.h"

using std::string;

namespace rosbag {

LZ4Stream::LZ4Stream(ChunkedFile* file)
    : Stream(file), block_size_id_(6) {
    buff_size_ = roslz4_blockSizeFromIndex(block_size_id_) + 64;
    buff_ = new char[buff_size_];
}

LZ4Stream::~LZ4Stream() {
    delete[] buff_;
}

CompressionType LZ4Stream::getCompressionType() const {
    return compression::LZ4;
}

void LZ4Stream::startWrite() {
    setCompressedIn(0);

    int ret = roslz4_compressStart(&lz4s_, block_size_id_);
    switch(ret) {
    case ROSLZ4_OK: break;
    case ROSLZ4_MEMORY_ERROR: throw BagIOException("ROSLZ4_MEMORY_ERROR: insufficient memory available"); break;
    case ROSLZ4_PARAM_ERROR: throw BagIOException("ROSLZ4_PARAM_ERROR: bad block size"); break;
    default: throw BagException("Unhandled return code");
    }
    lz4s_.output_next = buff_;
    lz4s_.output_left = buff_size_;
}

void LZ4Stream::write(void* ptr, size_t size) {
    lz4s_.input_left = static_cast<int>(size);
    lz4s_.input_next = (char*) ptr;

    writeStream(ROSLZ4_RUN);
    setCompressedIn(getCompressedIn() + size);
}

void LZ4Stream::writeStream(int action) {
    int ret = ROSLZ4_OK;
    while (lz4s_.input_left > 0 ||
           (action == ROSLZ4_FINISH && ret != ROSLZ4_STREAM_END)) {
        ret = roslz4_compress(&lz4s_, action);
        switch(ret) {
        case ROSLZ4_OK: break;
        case ROSLZ4_OUTPUT_SMALL:
            if (lz4s_.output_next - buff_ == buff_size_) {
                throw BagIOException("ROSLZ4_OUTPUT_SMALL: output buffer is too small");
            } else {
                // There's data to be written in buff_; this will free up space
                break;
            }
        case ROSLZ4_STREAM_END: break;
        case ROSLZ4_PARAM_ERROR: throw BagIOException("ROSLZ4_PARAM_ERROR: bad block size"); break;
        case ROSLZ4_ERROR: throw BagIOException("ROSLZ4_ERROR: compression error"); break;
        default: throw BagException("Unhandled return code");
        }

        // If output data is ready, write to disk
        int to_write = static_cast<int>(lz4s_.output_next - buff_);
        if (to_write > 0) {
            if (fwrite(buff_, 1, to_write, getFilePointer()) != static_cast<size_t>(to_write)) {
                throw BagException("Problem writing data to disk");
            }
            advanceOffset(to_write);
            lz4s_.output_next = buff_;
            lz4s_.output_left = buff_size_;
        }
    }
}

void LZ4Stream::stopWrite() {
    writeStream(ROSLZ4_FINISH);
    setCompressedIn(0);
    roslz4_compressEnd(&lz4s_);
}

void LZ4Stream::startRead() {
    int ret = static_cast<int>(roslz4_decompressStart(&lz4s_));
    switch(ret) {
    case ROSLZ4_OK: break;
    case ROSLZ4_MEMORY_ERROR: throw BagException("ROSLZ4_MEMORY_ERROR: insufficient memory available"); break;
    default: throw BagException("Unhandled return code");
    }

    if (getUnusedLength() > buff_size_) {
        throw BagException("Too many unused bytes to decompress");
    }

    // getUnused() could be pointing to part of buff_, so don't use memcpy
    memmove(buff_, getUnused(), getUnusedLength());
    lz4s_.input_next = buff_;
    lz4s_.input_left = getUnusedLength();
    clearUnused();
}

void LZ4Stream::read(void* ptr, size_t size) {
    // Setup stream by filling buffer with data from file
    int to_read = buff_size_ - lz4s_.input_left;
    char *input_start = buff_ + lz4s_.input_left;
    int nread = static_cast<int>(fread(input_start, 1, to_read, getFilePointer()));
    if (ferror(getFilePointer())) {
        throw BagIOException("Problem reading from file");
    }
    lz4s_.input_next = buff_;
    lz4s_.input_left += nread;
    lz4s_.output_next = (char*) ptr;
    lz4s_.output_left = static_cast<int>(size);

    // Decompress.  If reach end of stream, store unused data
    int ret = roslz4_decompress(&lz4s_);
    switch (ret) {
    case ROSLZ4_OK: break;
    case ROSLZ4_STREAM_END:
        if (getUnused() || getUnusedLength() > 0)
            CONSOLE_BRIDGE_logError("%s", "unused data already available");
        else {
            setUnused(lz4s_.input_next);
            setUnusedLength(lz4s_.input_left);
        }
        return;
    case ROSLZ4_ERROR: throw BagException("ROSLZ4_ERROR: decompression error"); break;
    case ROSLZ4_MEMORY_ERROR: throw BagException("ROSLZ4_MEMORY_ERROR: insufficient memory available"); break;
    case ROSLZ4_OUTPUT_SMALL: throw BagException("ROSLZ4_OUTPUT_SMALL: output buffer is too small"); break;
    case ROSLZ4_DATA_ERROR: throw BagException("ROSLZ4_DATA_ERROR: malformed data to decompress"); break;
    default: throw BagException("Unhandled return code");
    }
    if (feof(getFilePointer())) {
        throw BagIOException("Reached end of file before reaching end of stream");
    }

    size_t total_out = lz4s_.output_next - (char*)ptr;
    advanceOffset(total_out);

    // Shift input buffer if there's unconsumed data
    if (lz4s_.input_left > 0) {
        memmove(buff_, lz4s_.input_next, lz4s_.input_left);
    }
}

void LZ4Stream::stopRead() {
    roslz4_decompressEnd(&lz4s_);
}

void LZ4Stream::decompress(uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len) {
    unsigned int actual_dest_len = dest_len;
    int ret = roslz4_buffToBuffDecompress((char*)source, source_len,
                                          (char*)dest, &actual_dest_len);
    switch(ret) {
    case ROSLZ4_OK: break;
    case ROSLZ4_ERROR: throw BagException("ROSLZ4_ERROR: decompression error"); break;
    case ROSLZ4_MEMORY_ERROR: throw BagException("ROSLZ4_MEMORY_ERROR: insufficient memory available"); break;
    case ROSLZ4_OUTPUT_SMALL: throw BagException("ROSLZ4_OUTPUT_SMALL: output buffer is too small"); break;
    case ROSLZ4_DATA_ERROR: throw BagException("ROSLZ4_DATA_ERROR: malformed data to decompress"); break;
    default: throw BagException("Unhandled return code");
    }
    if (actual_dest_len != dest_len) {
        throw BagException("Decompression size mismatch in LZ4 chunk");
    }
}

} // namespace rosbag
