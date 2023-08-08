// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-topic-reader.h"

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <thread>


namespace realdds {


// A topic-reader that calls its callback from a separate thread.
// 
// This is the recommended way, according to eProsima:
//      "the following threads are spawned per participant:
//          - 1 for reception of UDP multicast discovery traffic of participants
//          - 1 for reception of UDP unicast discovery traffic of readers and writers
//          - 1 for reception of UDP unicast user traffic (common for all topics on the same participant)
//          - 1 for reception of SHM unicast user traffic (common for all topics on the same participant)"
// I.e., "on_data_available is called from the last two threads ... performing any lengthy process inside the callback
// is discouraged" and "calls to on_data_available are serialized".
// Experience also shows that one topic can starve out the others (e.g., metadata can get a lot more smaller messages
// than image topics), and handling the data in separate threads is the way to go.
// See also:
//      https://fast-dds.docs.eprosima.com/en/latest/fastdds/dds_layer/subscriber/dataReader/readingData.html#accessing-data-with-a-waiting-thread
//
class dds_topic_reader_thread : public dds_topic_reader
{
    typedef dds_topic_reader super;

    eprosima::fastdds::dds::GuardCondition _stopped;
    std::thread _th;

public:
    dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic );
    dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic,
                             std::shared_ptr< dds_subscriber > const & subscriber );
    ~dds_topic_reader_thread();

    void run( qos const & ) override;
    void stop() override;
};


}  // namespace realdds
