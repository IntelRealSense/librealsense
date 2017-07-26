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

#include "boost/format.hpp"

//#include <ros/ros.h>
#ifdef _WIN32
#    ifdef __MINGW32__
#      define fseeko fseeko64
#      define ftello ftello64
//     not sure if we need a ftruncate here yet or not
#    else
#        include <io.h>
#        define fseeko _fseeki64
#        define ftello _ftelli64
#        define fileno _fileno
#        define ftruncate _chsize_s //Intel Realsense Change, Was: #define ftruncate _chsize 
#    endif
#endif

using std::string;
using boost::format;
using std::shared_ptr;
using ros::Exception;

namespace rosbag {

ChunkedFile::ChunkedFile() :
    file_(NULL),
    offset_(0),
    compressed_in_(0),
    unused_(NULL),
    nUnused_(0)
{
    stream_factory_ = std::make_shared<StreamFactory>(this);
}

ChunkedFile::~ChunkedFile() {
    close();
}

void ChunkedFile::openReadWrite(string const& filename) { open(filename, "r+b"); }
void ChunkedFile::openWrite    (string const& filename) { open(filename, "w+b");  }
void ChunkedFile::openRead     (string const& filename) { open(filename, "rb");  }

void ChunkedFile::open(string const& filename, string const& mode) {
    // Check if file is already open
    if (file_)
        throw BagIOException((format("File already open: %1%") % filename_.c_str()).str());

    // Open the file
    if (mode == "r+b") {
        // check if file already exists
        #if defined(_MSC_VER) && (_MSC_VER >= 1400 )
            fopen_s( &file_, filename.c_str(), "r" );
        #else
            file_ = fopen(filename.c_str(), "r");
        #endif
        if (file_ == NULL)
            // create an empty file and open it for update
            #if defined(_MSC_VER) && (_MSC_VER >= 1400 )
                fopen_s( &file_, filename.c_str(), "w+b" );
            #else
                file_ = fopen(filename.c_str(), "w+b");
            #endif
        else {
            fclose(file_);
            // open existing file for update
            #if defined(_MSC_VER) && (_MSC_VER >= 1400 )
                fopen_s( &file_, filename.c_str(), "r+b" );
            #else
                file_ = fopen(filename.c_str(), "r+b");
            #endif
        }
    }
    else
        #if defined(_MSC_VER) && (_MSC_VER >= 1400 )
            fopen_s( &file_, filename.c_str(), mode.c_str() );
        #else
            file_ = fopen(filename.c_str(), mode.c_str());
        #endif

    if (!file_)
        throw BagIOException((format("Error opening file: %1%") % filename.c_str()).str());

    read_stream_  = std::make_shared<UncompressedStream>(this);
    write_stream_ = std::make_shared<UncompressedStream>(this);
    filename_     = filename;
    offset_       = ftello(file_);
}

bool ChunkedFile::good() const {
    return feof(file_) == 0 && ferror(file_) == 0;
}

bool   ChunkedFile::isOpen()      const { return file_ != NULL; }
string ChunkedFile::getFileName() const { return filename_;     }

void ChunkedFile::close() {
    if (!file_)
        return;

    // Close any compressed stream by changing to uncompressed mode
    setWriteMode(compression::Uncompressed);

    // Close the file
    int success = fclose(file_);
    if (success != 0)
        throw BagIOException((format("Error closing file: %1%") % filename_.c_str()).str());

    file_ = NULL;
    filename_.clear();
    
    clearUnused();
}

// Read/write modes

void ChunkedFile::setWriteMode(CompressionType type) {
    if (!file_)
        throw BagIOException("Can't set compression mode before opening a file");

    if (type != write_stream_->getCompressionType()) {
        write_stream_->stopWrite();
        shared_ptr<Stream> stream = stream_factory_->getStream(type);
        stream->startWrite();
        write_stream_ = stream;
    }
}

void ChunkedFile::setReadMode(CompressionType type) {
    if (!file_)
        throw BagIOException("Can't set compression mode before opening a file");

    if (type != read_stream_->getCompressionType()) {
        read_stream_->stopRead();
        shared_ptr<Stream> stream = stream_factory_->getStream(type);
        stream->startRead();
        read_stream_ = stream;
    }
}

void ChunkedFile::seek(uint64_t offset, int origin) {
    if (!file_)
        throw BagIOException("Can't seek - file not open");

    setReadMode(compression::Uncompressed);

    int success = fseeko(file_, offset, origin);
    if (success != 0)
        throw BagIOException("Error seeking");

    offset_ = ftello(file_);
}

uint64_t ChunkedFile::getOffset()            const { return offset_;        }
uint32_t ChunkedFile::getCompressedBytesIn() const { return static_cast<uint32_t>(compressed_in_); }

void ChunkedFile::write(string const& s)        { write((void*) s.c_str(), s.size()); }
void ChunkedFile::write(void* ptr, size_t size) { write_stream_->write(ptr, size);    }
void ChunkedFile::read(void* ptr, size_t size)  { read_stream_->read(ptr, size);      }

bool ChunkedFile::truncate(uint64_t length) {
    int fd = fileno(file_);
    return ftruncate(fd, length) == 0;
}

//! \todo add error handling
string ChunkedFile::getline() {
    char buffer[1024];
    if(fgets(buffer, 1024, file_))
    {
      string s(buffer);
      offset_ += s.size();
      return s;
    }
    else
      return string("");
}

void ChunkedFile::decompress(CompressionType compression, uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len) {
    stream_factory_->getStream(compression)->decompress(dest, dest_len, source, source_len);
}

void ChunkedFile::clearUnused() {
    unused_ = NULL;
    nUnused_ = 0;
}

} // namespace rosbag
