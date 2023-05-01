// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/topics/flexible-msg.h>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <string>
#include <memory>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SubscriptionMatchedStatus;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {


// Ease-of-use helper, easily taking care of basic use-cases that just want to easily read from some topic.
// E.g.:
//     flexible_reader topic( participant, "topic-name" );
//     ...
//     auto msg = topic.read().msg;
//
class flexible_reader
{
    std::shared_ptr< dds_topic_reader > _reader;
    int _n_writers = 0;

public:
    // We need to return both a message and the sample that goes with it, if needed
    struct data_t
    {
        flexible_msg msg;
        eprosima::fastdds::dds::SampleInfo sample;
    };

private:
    std::queue< data_t > _data;
    std::condition_variable _data_cv;
    std::mutex _data_mutex;

public:
    flexible_reader( std::shared_ptr< dds_topic > const & topic );
    flexible_reader( std::shared_ptr< dds_participant > const & participant, std::string const & topic_name )
        : flexible_reader( flexible_msg::create_topic( participant, topic_name ) )
    {
    }

    std::string const & name() const;

    // Block until data is available
    void wait_for_data();
    // Block, but throw on timeout
    void wait_for_data( std::chrono::seconds timeout );

    // Blocking -- waits until data is available
    data_t read();
    // Blocking -- but with a timeout (throws)
    data_t read( std::chrono::seconds timeout );

    bool empty() const { return _data.empty(); }

private:
    void on_subscription_matched( eprosima::fastdds::dds::SubscriptionMatchedStatus const & );
    void on_data_available();
};


}  // namespace topics
}  // namespace realdds
