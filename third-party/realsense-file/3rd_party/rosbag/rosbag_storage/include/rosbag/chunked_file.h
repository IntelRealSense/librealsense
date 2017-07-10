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

#ifndef ROSBAG_CHUNKED_FILE_H
#define ROSBAG_CHUNKED_FILE_H

#include <ios>
#include <stdint.h>
#include <string>
#include "macros.h"
//#include <boost/shared_ptr.hpp>

#include "../../../rosbag_storage/include/rosbag/stream.h"

namespace rosbag {

//! ChunkedFile reads and writes files which contain interleaved chunks of compressed and uncompressed data.
class ROSBAG_DECL ChunkedFile
{
    friend class Stream;

public:
    ChunkedFile();
    ~ChunkedFile();

    void openWrite    (std::string const& filename);            //!< open file for writing
    void openRead     (std::string const& filename);            //!< open file for reading
    void openReadWrite(std::string const& filename);            //!< open file for reading & writing

    void close();                                               //!< close the file

    std::string getFileName()          const;                   //!< return path of currently open file
    uint64_t    getOffset()            const;                   //!< return current offset from the beginning of the file
    uint32_t    getCompressedBytesIn() const;                   //!< return the number of bytes written to current compressed stream
    bool        isOpen()               const;                   //!< return true if file is open for reading or writing
    bool        good()                 const;                   //!< return true if hasn't reached end-of-file and no error

    void        setReadMode(CompressionType type);
    void        setWriteMode(CompressionType type);

    // File I/O
    void        write(std::string const& s);
    void        write(void* ptr, size_t size);                          //!< write size bytes from ptr to the file
    void        read(void* ptr, size_t size);                           //!< read size bytes from the file into ptr
    std::string getline();
    bool        truncate(uint64_t length);
    void        seek(uint64_t offset, int origin = std::ios_base::beg); //!< seek to given offset from origin
    void        decompress(CompressionType compression, uint8_t* dest, unsigned int dest_len, uint8_t* source, unsigned int source_len);

private:
    void open(std::string const& filename, std::string const& mode);
    void clearUnused();

private:
    std::string filename_;       //!< path to file
    FILE*       file_;           //!< file pointer
    uint64_t    offset_;         //!< current position in the file
    uint64_t    compressed_in_;  //!< number of bytes written to current compressed stream
    char*       unused_;         //!< extra data read by compressed stream
    int         nUnused_;        //!< number of bytes of extra data read by compressed stream

	std::shared_ptr<StreamFactory> stream_factory_;

	std::shared_ptr<Stream> read_stream_;
	std::shared_ptr<Stream> write_stream_;
};

} // namespace rosbag

#endif
