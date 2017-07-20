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

#ifndef ROSBAG_STREAM_H
#define ROSBAG_STREAM_H

#include <ios>
#include <stdint.h>
#include <string>

//#include <boost/shared_ptr.hpp>
#include <memory>
#include "../../../roslz4/include/roslz4/lz4s.h"

#include "../../../rosbag_storage/include/rosbag/exceptions.h"
#include "../../../rosbag_storage/include/rosbag/macros.h"

namespace rosbag {

namespace compression
{
    enum CompressionType
    {
        Uncompressed = 0,
        BZ2          = 1,
        LZ4          = 2,
    };
}
typedef compression::CompressionType CompressionType;

class ChunkedFile;

class ROSBAG_DECL Stream
{
public:
    Stream(ChunkedFile* file);
    virtual ~Stream();

    virtual CompressionType getCompressionType() const = 0;

    virtual void write(void* ptr, size_t size) = 0;
    virtual void read (void* ptr, size_t size) = 0;

    virtual void decompress(uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len) = 0;

    virtual void startWrite();
    virtual void stopWrite();

    virtual void startRead();
    virtual void stopRead();

protected:
    FILE*    getFilePointer();
    uint64_t getCompressedIn();
    void     setCompressedIn(uint64_t nbytes);
    void     advanceOffset(uint64_t nbytes);
    char*    getUnused();
    int      getUnusedLength();
    void     setUnused(char* unused);
    void     setUnusedLength(int nUnused);
    void     clearUnused();

protected:
    ChunkedFile* file_;
};

class ROSBAG_DECL StreamFactory
{
public:
    StreamFactory(ChunkedFile* file);

	std::shared_ptr<Stream> getStream(CompressionType type) const;

private:
    std::shared_ptr<Stream> uncompressed_stream_;
    std::shared_ptr<Stream> lz4_stream_;
};

class ROSBAG_DECL UncompressedStream : public Stream
{
public:
    UncompressedStream(ChunkedFile* file);

    CompressionType getCompressionType() const;

    void write(void* ptr, size_t size);
    void read(void* ptr, size_t size);

    void decompress(uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len);
};

// LZ4Stream reads/writes compressed datat in the LZ4 format
// https://code.google.com/p/lz4/
class ROSBAG_DECL LZ4Stream : public Stream
{
public:
    LZ4Stream(ChunkedFile* file);
    ~LZ4Stream();

    CompressionType getCompressionType() const;

    void startWrite();
    void write(void* ptr, size_t size);
    void stopWrite();

    void startRead();
    void read(void* ptr, size_t size);
    void stopRead();

    void decompress(uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len);

private:
    void writeStream(int action);

    char *buff_;
    int buff_size_;
    int block_size_id_;
    roslz4_stream lz4s_;
};



} // namespace rosbag

#endif
