// Copyright (c) 2009, Willow Garage, Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Willow Garage, Inc. nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "rosbag/message_instance.h"

#include "ros/message_event.h"

using std::string;
using ros::Time;
using std::shared_ptr;

namespace rosbag {

MessageInstance::MessageInstance(ConnectionInfo const* connection_info, IndexEntry const& index_entry, Bag const& bag) :
	connection_info_(connection_info), index_entry_(index_entry), bag_(&bag)
{
}

Time const&   MessageInstance::getTime()              const { return index_entry_.time;          }
string const& MessageInstance::getTopic()             const { return connection_info_->topic;    }
string const& MessageInstance::getDataType()          const { return connection_info_->datatype; }
string const& MessageInstance::getMD5Sum()            const { return connection_info_->md5sum;   }
string const& MessageInstance::getMessageDefinition() const { return connection_info_->msg_def;  }

shared_ptr<ros::M_string> MessageInstance::getConnectionHeader() const { return connection_info_->header; }

string MessageInstance::getCallerId() const {
    ros::M_string::const_iterator header_iter = connection_info_->header->find("callerid");
    return header_iter != connection_info_->header->end() ? header_iter->second : string("");
}

bool MessageInstance::isLatching() const {
    ros::M_string::const_iterator header_iter = connection_info_->header->find("latching");
    return header_iter != connection_info_->header->end() && header_iter->second == "1";
}

uint32_t MessageInstance::size() const {
    return bag_->readMessageDataSize(index_entry_);
}

} // namespace rosbag
