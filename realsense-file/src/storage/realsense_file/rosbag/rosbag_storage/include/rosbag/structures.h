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

#ifndef ROSBAG_STRUCTURES_H
#define ROSBAG_STRUCTURES_H

#include <map>
#include <vector>

#include "ros/time.h"
#include "ros/datatypes.h"
#include "macros.h"

namespace rosbag {

struct ROSBAG_DECL ConnectionInfo
{
    ConnectionInfo() : id(-1) { }

    uint32_t    id;
    std::string topic;
    std::string datatype;
    std::string md5sum;
    std::string msg_def;

    boost::shared_ptr<ros::M_string> header;
};

struct ChunkInfo
{
    ros::Time   start_time;    //! earliest timestamp of a message in the chunk
    ros::Time   end_time;      //! latest timestamp of a message in the chunk
    uint64_t    pos;           //! absolute byte offset of chunk record in bag file

    std::map<uint32_t, uint32_t> connection_counts;   //! number of messages in each connection stored in the chunk
};

struct ROSBAG_DECL ChunkHeader
{
    std::string compression;          //! chunk compression type, e.g. "none" or "bz2" (see constants.h)
    uint32_t    compressed_size;      //! compressed size of the chunk in bytes
    uint32_t    uncompressed_size;    //! uncompressed size of the chunk in bytes
};

struct ROSBAG_DECL IndexEntry
{
    ros::Time time;            //! timestamp of the message
    uint64_t  chunk_pos;       //! absolute byte offset of the chunk record containing the message
    uint32_t  offset;          //! relative byte offset of the message record (either definition or data) in the chunk

    bool operator<(IndexEntry const& b) const { return time < b.time; }
};

struct ROSBAG_DECL IndexEntryCompare
{
    bool operator()(ros::Time const& a, IndexEntry const& b) const { return a < b.time; }
    bool operator()(IndexEntry const& a, ros::Time const& b) const { return a.time < b; }
};

} // namespace rosbag

#endif
