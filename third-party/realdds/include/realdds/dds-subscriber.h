// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>


namespace eprosima {
namespace fastdds {
namespace dds {
class Subscriber;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;


// The Subscriber manages the activities of several dds_topic_reader (DataReader) entities
//
class dds_subscriber
{
    std::shared_ptr< dds_participant > _participant;

    eprosima::fastdds::dds::Subscriber * _subscriber;

public:
    dds_subscriber( std::shared_ptr< dds_participant > const & participant );
    ~dds_subscriber();

    eprosima::fastdds::dds::Subscriber * get() const { return _subscriber; }
    eprosima::fastdds::dds::Subscriber * operator->() const { return get(); }

    std::shared_ptr< dds_participant > const & get_participant() const { return _participant; }
};


}  // namespace realdds
