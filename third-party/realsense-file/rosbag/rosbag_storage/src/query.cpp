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

#include "rosbag/query.h"
#include "rosbag/bag.h"

#include <boost/foreach.hpp>

#define foreach BOOST_FOREACH

using std::map;
using std::string;
using std::vector;
using std::multiset;

namespace rosbag {

// Query

Query::Query(boost::function<bool(ConnectionInfo const*)>& query, ros::Time const& start_time, ros::Time const& end_time)
	: query_(query), start_time_(start_time), end_time_(end_time)
{
}

boost::function<bool(ConnectionInfo const*)> const& Query::getQuery() const {
	return query_;
}

ros::Time const& Query::getStartTime() const { return start_time_; }
ros::Time const& Query::getEndTime()   const { return end_time_;   }

// TopicQuery

TopicQuery::TopicQuery(std::string const& topic) {
    topics_.push_back(topic);
}

TopicQuery::TopicQuery(std::vector<std::string> const& topics) : topics_(topics) { }

bool TopicQuery::operator()(ConnectionInfo const* info) const {
    foreach(string const& topic, topics_)
        if (topic == info->topic)
            return true;

    return false;
}

// TypeQuery

TypeQuery::TypeQuery(std::string const& type) {
    types_.push_back(type);
}

TypeQuery::TypeQuery(std::vector<std::string> const& types) : types_(types) { }

bool TypeQuery::operator()(ConnectionInfo const* info) const {
    foreach(string const& type, types_)
        if (type == info->datatype)
            return true;

    return false;
}

// BagQuery

BagQuery::BagQuery(Bag const* _bag, Query const& _query, uint32_t _bag_revision) : bag(_bag), query(_query), bag_revision(_bag_revision) {
}

// MessageRange

MessageRange::MessageRange(std::multiset<IndexEntry>::const_iterator const& _begin,
                           std::multiset<IndexEntry>::const_iterator const& _end,
                           ConnectionInfo const* _connection_info,
                           BagQuery const* _bag_query)
	: begin(_begin), end(_end), connection_info(_connection_info), bag_query(_bag_query)
{
}

// ViewIterHelper

ViewIterHelper::ViewIterHelper(std::multiset<IndexEntry>::const_iterator _iter, MessageRange const* _range)
	: iter(_iter), range(_range)
{
}

bool ViewIterHelperCompare::operator()(ViewIterHelper const& a, ViewIterHelper const& b) {
	return (a.iter)->time > (b.iter)->time;
}

} // namespace rosbag
