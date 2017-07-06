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
*********************************************************************/

#ifndef ROSBAG_CONSTANTS_H
#define ROSBAG_CONSTANTS_H

#include <string>
#include <stdint.h>

namespace rosbag {

// Bag file version to write
static const std::string VERSION = "2.0";

// Header field delimiter
static const unsigned char FIELD_DELIM = '=';

// Current header fields
static const std::string OP_FIELD_NAME               = "op";
static const std::string TOPIC_FIELD_NAME            = "topic";
static const std::string VER_FIELD_NAME              = "ver";
static const std::string COUNT_FIELD_NAME            = "count";
static const std::string INDEX_POS_FIELD_NAME        = "index_pos";     // 1.2+
static const std::string CONNECTION_COUNT_FIELD_NAME = "conn_count";    // 2.0+
static const std::string CHUNK_COUNT_FIELD_NAME      = "chunk_count";   // 2.0+
static const std::string CONNECTION_FIELD_NAME       = "conn";          // 2.0+
static const std::string COMPRESSION_FIELD_NAME      = "compression";   // 2.0+
static const std::string SIZE_FIELD_NAME             = "size";          // 2.0+
static const std::string TIME_FIELD_NAME             = "time";          // 2.0+
static const std::string START_TIME_FIELD_NAME       = "start_time";    // 2.0+
static const std::string END_TIME_FIELD_NAME         = "end_time";      // 2.0+
static const std::string CHUNK_POS_FIELD_NAME        = "chunk_pos";     // 2.0+

// Legacy header fields
static const std::string MD5_FIELD_NAME      = "md5";           // <2.0
static const std::string TYPE_FIELD_NAME     = "type";          // <2.0
static const std::string DEF_FIELD_NAME      = "def";           // <2.0
static const std::string SEC_FIELD_NAME      = "sec";           // <2.0
static const std::string NSEC_FIELD_NAME     = "nsec";          // <2.0
static const std::string LATCHING_FIELD_NAME = "latching";      // <2.0
static const std::string CALLERID_FIELD_NAME = "callerid";      // <2.0

// Current "op" field values
static const unsigned char OP_MSG_DATA    = 0x02;
static const unsigned char OP_FILE_HEADER = 0x03;
static const unsigned char OP_INDEX_DATA  = 0x04;
static const unsigned char OP_CHUNK       = 0x05;
static const unsigned char OP_CHUNK_INFO  = 0x06;
static const unsigned char OP_CONNECTION  = 0x07;

// Legacy "op" field values
static const unsigned char OP_MSG_DEF     = 0x01;

// Bytes reserved for file header record (4KB)
static const uint32_t FILE_HEADER_LENGTH = 4 * 1024;

// Index data record version to write
static const uint32_t INDEX_VERSION = 1;

// Chunk info record version to write
static const uint32_t CHUNK_INFO_VERSION = 1;

// Compression types
static const std::string COMPRESSION_NONE = "none";
static const std::string COMPRESSION_BZ2  = "bz2";
static const std::string COMPRESSION_LZ4  = "lz4";

} // namespace rosbag

#endif
