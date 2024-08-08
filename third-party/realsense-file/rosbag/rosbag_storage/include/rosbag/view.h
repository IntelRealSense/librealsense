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

#ifndef ROSBAG_VIEW_H
#define ROSBAG_VIEW_H

#include <memory.h>
#include <iterator>
#include <functional>

#include "rosbag/message_instance.h"
#include "rosbag/query.h"
#include "rosbag/macros.h"
#include "rosbag/structures.h"

namespace rosbag {

class ROSBAG_DECL View
{
    friend class Bag;

public:
    //! An iterator that points to a MessageInstance from a bag
    /*!
     * This iterator stores the MessageInstance that it is returning a
     * reference to.  If you increment the iterator that
     * MessageInstance is destroyed.  You should never store the
     * pointer to this reference.
     */
    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = MessageInstance;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

    public:
        iterator( iterator const & i );
        iterator & operator=( iterator const & i );
        iterator();
        ~iterator();

        iterator& operator++() { increment(); return *this; }
        iterator operator++(int) { iterator retval = *this; increment(); return retval; }
        bool operator==(iterator const & other) const { return equal( other ); }
        bool operator!=(iterator const & other) const { return ! equal( other ); }
        reference operator*() const { return dereference(); }

    protected:
        iterator( View * view, bool end = false );

    private:
        friend class View;

        void populate();
        void populateSeek( std::multiset< IndexEntry >::const_iterator iter );

        bool equal( iterator const & other ) const;

        void increment();

        MessageInstance & dereference() const;

    private:
        View* view_;
        std::vector< ViewIterHelper > iters_;
        uint32_t view_revision_;
        mutable MessageInstance* message_instance_;
    };

    typedef iterator const_iterator;

    struct TrueQuery {
    	bool operator()(ConnectionInfo const*) const { return true; };
    };

    //! Create a view on a bag
    /*!
     * param reduce_overlap  If multiple views return the same messages, reduce them to a single message
     */
    View(bool const& reduce_overlap = false);

    //! Create a view on a bag
    /*!
     * param bag             The bag file on which to run this query
     * param start_time      The beginning of the time range for the query
     * param end_time        The end of the time range for the query
     * param reduce_overlap  If multiple views return the same messages, reduce them to a single message
     */
    View(Bag const& bag, rs2rosinternal::Time const& start_time = rs2rosinternal::TIME_MIN, rs2rosinternal::Time const& end_time = rs2rosinternal::TIME_MAX, bool const& reduce_overlap = false);

    //! Create a view and add a query
    /*!
     * param bag             The bag file on which to run this query
     * param query           The actual query to evaluate which connections to include
     * param start_time      The beginning of the time range for the query
     * param end_time        The end of the time range for the query
     * param reduce_overlap  If multiple views return the same messages, reduce them to a single message
     */
    View(Bag const& bag, std::function<bool(ConnectionInfo const*)> query,
         rs2rosinternal::Time const& start_time = rs2rosinternal::TIME_MIN, rs2rosinternal::Time const& end_time = rs2rosinternal::TIME_MAX, bool const& reduce_overlap = false);

    ~View();

    iterator begin();
    iterator end();
    uint32_t size();

    //! Add a query to a view
    /*!
     * param bag        The bag file on which to run this query
     * param start_time The beginning of the time range for the query
     * param end_time   The end of the time range for the query
     */
    void addQuery(Bag const& bag, rs2rosinternal::Time const& start_time = rs2rosinternal::TIME_MIN, rs2rosinternal::Time const& end_time = rs2rosinternal::TIME_MAX);

    //! Add a query to a view
    /*!
     * param bag        The bag file on which to run this query
     * param query      The actual query to evaluate which connections to include
     * param start_time The beginning of the time range for the query
     * param end_time   The end of the time range for the query
     */
    void addQuery(Bag const& bag, std::function<bool(ConnectionInfo const*)> query,
    		      rs2rosinternal::Time const& start_time = rs2rosinternal::TIME_MIN, rs2rosinternal::Time const& end_time = rs2rosinternal::TIME_MAX);

    std::vector<const ConnectionInfo*> getConnections();

    rs2rosinternal::Time getBeginTime();
    rs2rosinternal::Time getEndTime();
  
protected:
    friend class iterator;

    void updateQueries(BagQuery* q);
    void update();

    MessageInstance* newMessageInstance(ConnectionInfo const* connection_info, IndexEntry const& index, Bag const& bag);

private:
    View(View const& view);
    View& operator=(View const& view);

protected:
    std::vector<MessageRange*> ranges_;
    std::vector<BagQuery*>     queries_;
    uint32_t                   view_revision_;

    uint32_t size_cache_;
    uint32_t size_revision_;

    bool reduce_overlap_;
};

} // namespace rosbag

#endif
