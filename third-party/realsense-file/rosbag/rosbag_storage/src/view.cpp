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

#include "rosbag/view.h"
#include "rosbag/bag.h"
#include "rosbag/message_instance.h"

#include <boost/foreach.hpp>
#include <set>
#include <assert.h>

#define foreach BOOST_FOREACH

using std::map;
using std::string;
using std::vector;
using std::multiset;

namespace rosbag {

// View::iterator

View::iterator::iterator() : view_(NULL), view_revision_(0), message_instance_(NULL) { }

View::iterator::~iterator()
{
  if (message_instance_ != NULL)
    delete message_instance_;
}

View::iterator::iterator(View* view, bool end) : view_(view), view_revision_(0), message_instance_(NULL) {
    if (view != NULL && !end)
        populate();
}

View::iterator::iterator(const iterator& i) : view_(i.view_), iters_(i.iters_), view_revision_(i.view_revision_), message_instance_(NULL) { }

View::iterator &View::iterator::operator=(iterator const& i) {
    if (this != &i) {
        view_ = i.view_;
        iters_ = i.iters_;
        view_revision_ = i.view_revision_;
        if (message_instance_ != NULL) {
            delete message_instance_;
            message_instance_ = NULL;
        }
    }
    return *this;
}

void View::iterator::populate() {
    assert(view_ != NULL);

    iters_.clear();
    foreach(MessageRange const* range, view_->ranges_)
        if (range->begin != range->end)
            iters_.push_back(ViewIterHelper(range->begin, range));

    std::sort(iters_.begin(), iters_.end(), ViewIterHelperCompare());
    view_revision_ = view_->view_revision_;
}

void View::iterator::populateSeek(multiset<IndexEntry>::const_iterator iter) {
    assert(view_ != NULL);

    iters_.clear();
    foreach(MessageRange const* range, view_->ranges_) {
        multiset<IndexEntry>::const_iterator start = std::lower_bound(range->begin, range->end, iter->time, IndexEntryCompare());
        if (start != range->end)
            iters_.push_back(ViewIterHelper(start, range));
    }

    std::sort(iters_.begin(), iters_.end(), ViewIterHelperCompare());
    while (iter != iters_.back().iter)
        increment();

    view_revision_ = view_->view_revision_;
}

bool View::iterator::equal(View::iterator const& other) const {
    // We need some way of verifying these are actually talking about
    // the same merge_queue data since we shouldn't be able to compare
    // iterators from different Views.

    if (iters_.empty())
        return other.iters_.empty();
    if (other.iters_.empty())
        return false;

    return iters_.back().iter == other.iters_.back().iter;
}

void View::iterator::increment() {
    assert(view_ != NULL);

    // Our message instance is no longer valid
    if (message_instance_ != NULL)
    {
      delete message_instance_;
      message_instance_ = NULL;
    }

    view_->update();

    // Note, updating may have blown away our message-ranges and
    // replaced them in general the ViewIterHelpers are no longer
    // valid, but the iterator it stores should still be good.
    if (view_revision_ != view_->view_revision_)
        populateSeek(iters_.back().iter);

    if (view_->reduce_overlap_)
    {
        std::multiset<IndexEntry>::const_iterator last_iter = iters_.back().iter;
    
        while (iters_.back().iter == last_iter)
        {
            iters_.back().iter++;
            if (iters_.back().iter == iters_.back().range->end)
                iters_.pop_back();
      
            std::sort(iters_.begin(), iters_.end(), ViewIterHelperCompare());
        }

    } else {

        iters_.back().iter++;
        if (iters_.back().iter == iters_.back().range->end)
            iters_.pop_back();
      
        std::sort(iters_.begin(), iters_.end(), ViewIterHelperCompare());
    }
}

MessageInstance& View::iterator::dereference() const {
    ViewIterHelper const& i = iters_.back();

    if (message_instance_ == NULL)
      message_instance_ = view_->newMessageInstance(i.range->connection_info, *(i.iter), *(i.range->bag_query->bag));

    return *message_instance_;
}

// View

View::View(bool const& reduce_overlap) : view_revision_(0), size_cache_(0), size_revision_(0), reduce_overlap_(reduce_overlap) { }

View::View(Bag const& bag, rs2rosinternal::Time const& start_time, rs2rosinternal::Time const& end_time, bool const& reduce_overlap) : view_revision_(0), size_cache_(0), size_revision_(0), reduce_overlap_(reduce_overlap) {
	addQuery(bag, start_time, end_time);
}

View::View(Bag const& bag, boost::function<bool(ConnectionInfo const*)> query, rs2rosinternal::Time const& start_time, rs2rosinternal::Time const& end_time, bool const& reduce_overlap) : view_revision_(0), size_cache_(0), size_revision_(0), reduce_overlap_(reduce_overlap) {
	addQuery(bag, query, start_time, end_time);
}

View::~View() {
    foreach(MessageRange* range, ranges_)
        delete range;
    foreach(BagQuery* query, queries_)
        delete query;
}


rs2rosinternal::Time View::getBeginTime()
{
  update();

  rs2rosinternal::Time begin = rs2rosinternal::TIME_MAX;

  foreach (rosbag::MessageRange* range, ranges_)
  {
    if (range->begin->time < begin)
      begin = range->begin->time;
  }

  return begin;
}

rs2rosinternal::Time View::getEndTime()
{
  update();

  rs2rosinternal::Time end = rs2rosinternal::TIME_MIN;

  foreach (rosbag::MessageRange* range, ranges_)
  {
    std::multiset<IndexEntry>::const_iterator e = range->end;
    e--;

    if (e->time > end)
      end = e->time;
  }

  return end;
}

//! Simply copy the merge_queue state into the iterator
View::iterator View::begin() {
    update();
    return iterator(this);
}

//! Default constructed iterator signifies end
View::iterator View::end() { return iterator(this, true); }

uint32_t View::size() { 

  update();

  if (size_revision_ != view_revision_)
  {
    size_cache_ = 0;

    foreach (MessageRange* range, ranges_)
    {
      size_cache_ += static_cast<uint32_t>(std::distance(range->begin, range->end));
    }

    size_revision_ = view_revision_;
  }

  return size_cache_;
}

void View::addQuery(Bag const& bag, rs2rosinternal::Time const& start_time, rs2rosinternal::Time const& end_time) {
    if ((bag.getMode() & bagmode::Read) != bagmode::Read)
        throw BagException("Bag not opened for reading");

	boost::function<bool(ConnectionInfo const*)> query = TrueQuery();

    queries_.push_back(new BagQuery(&bag, Query(query, start_time, end_time), bag.bag_revision_));

    updateQueries(queries_.back());
}

void View::addQuery(Bag const& bag, boost::function<bool(ConnectionInfo const*)> query, rs2rosinternal::Time const& start_time, rs2rosinternal::Time const& end_time) {
    if ((bag.getMode() & bagmode::Read) != bagmode::Read)
        throw BagException("Bag not opened for reading");

    queries_.push_back(new BagQuery(&bag, Query(query, start_time, end_time), bag.bag_revision_));

    updateQueries(queries_.back());
}

void View::updateQueries(BagQuery* q) {
    for (map<uint32_t, ConnectionInfo*>::const_iterator i = q->bag->connections_.begin(); i != q->bag->connections_.end(); i++) {
        ConnectionInfo const* connection = i->second;

        // Skip if the query doesn't evaluate to true
        if (!q->query.getQuery()(connection))
            continue;

        map<uint32_t, multiset<IndexEntry> >::const_iterator j = q->bag->connection_indexes_.find(connection->id);

        // Skip if the bag doesn't have the corresponding index
        if (j == q->bag->connection_indexes_.end())
            continue;
        multiset<IndexEntry> const& index = j->second;

        // lower_bound/upper_bound do a binary search to find the appropriate range of Index Entries given our time range

        std::multiset<IndexEntry>::const_iterator begin = index.lower_bound({ q->query.getStartTime(), 0, 0 });
        std::multiset<IndexEntry>::const_iterator end   = index.upper_bound({ q->query.getEndTime()  , 0, 0 });

        // Make sure we are at the right beginning
        while (begin != index.begin())
        {
			if (begin != index.end() && begin->time >= q->query.getStartTime())
				break;

          begin--;
          if (begin->time < q->query.getStartTime())
          {
            begin++;
            break;
          }
        }

        if (begin != end)
        {
            // todo: make faster with a map of maps
            bool found = false;
            for (vector<MessageRange*>::iterator k = ranges_.begin(); k != ranges_.end(); k++) {
                MessageRange* r = *k;
                
                // If the topic and query are already in our ranges, we update
                if (r->bag_query == q && r->connection_info->id == connection->id) {
                    r->begin = begin;
                    r->end   = end;
                    found    = true;
                    break;
                }
            }
            if (!found)
                ranges_.push_back(new MessageRange(begin, end, connection, q));
        }
    }

    view_revision_++;
}

void View::update() {
    foreach(BagQuery* query, queries_) {
        if (query->bag->bag_revision_ != query->bag_revision) {
            updateQueries(query);
            query->bag_revision = query->bag->bag_revision_;
        }
    }
}

std::vector<const ConnectionInfo*> View::getConnections()
{
  std::vector<const ConnectionInfo*> connections;

  foreach(MessageRange* range, ranges_)
  {
    connections.push_back(range->connection_info);
  }

  return connections;
}

MessageInstance* View::newMessageInstance(ConnectionInfo const* connection_info, IndexEntry const& index, Bag const& bag)
{
  return new MessageInstance(connection_info, index, bag);
}


} // namespace rosbag
