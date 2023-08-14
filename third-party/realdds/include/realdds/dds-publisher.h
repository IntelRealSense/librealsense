// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>


namespace eprosima {
namespace fastdds {
namespace dds {
class Publisher;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;


// The Publisher manages the activities of several dds_topic_writer (DataWriter) entities
//
class dds_publisher
{
    std::shared_ptr< dds_participant > _participant;

    eprosima::fastdds::dds::Publisher * _publisher;

public:
    dds_publisher( std::shared_ptr< dds_participant > const & participant );
    ~dds_publisher();

    eprosima::fastdds::dds::Publisher * get() const { return _publisher; }
    eprosima::fastdds::dds::Publisher * operator->() const { return get(); }

    std::shared_ptr< dds_participant > const & get_participant() const { return _participant; }
};


}  // namespace realdds
