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

#include "rosbag/stream.h"
#include "rosbag/chunked_file.h"

//#include <ros/ros.h>

using std::shared_ptr;

namespace rosbag {

// StreamFactory

StreamFactory::StreamFactory(ChunkedFile* file) :
    uncompressed_stream_(new UncompressedStream(file)),
    lz4_stream_         (new LZ4Stream(file))
{
}

shared_ptr<Stream> StreamFactory::getStream(CompressionType type) const {
    switch (type) {
        case compression::Uncompressed: return uncompressed_stream_;
        case compression::LZ4:          return lz4_stream_;
        default:                        return shared_ptr<Stream>();
    }
}

// Stream

Stream::Stream(ChunkedFile* file) : file_(file) { }

Stream::~Stream() { }

void Stream::startWrite() { }
void Stream::stopWrite()  { }
void Stream::startRead()  { }
void Stream::stopRead()   { }

FILE*    Stream::getFilePointer()                 { return file_->file_;            }
uint64_t Stream::getCompressedIn()                { return file_->compressed_in_;   }
void     Stream::setCompressedIn(uint64_t nbytes) { file_->compressed_in_ = nbytes; }
void     Stream::advanceOffset(uint64_t nbytes)   { file_->offset_ += nbytes;       }
char*    Stream::getUnused()                      { return file_->unused_;          }
int      Stream::getUnusedLength()                { return file_->nUnused_;         }
void     Stream::setUnused(char* unused)          { file_->unused_ = unused;        }
void     Stream::setUnusedLength(int nUnused)     { file_->nUnused_ = nUnused;      }
void     Stream::clearUnused()                    { file_->clearUnused();           }

} // namespace rosbag
