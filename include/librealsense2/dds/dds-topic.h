// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>


namespace eprosima {
namespace fastdds {
namespace dds {
class Topic;
class TypeSupport;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace librealsense {
namespace dds {


class dds_participant;


class dds_topic
{
    std::shared_ptr< dds_participant > const _participant;
    eprosima::fastdds::dds::Topic * _topic;

public:
    dds_topic( std::shared_ptr< dds_participant > const & participant,
               eprosima::fastdds::dds::TypeSupport const & topic_type,
               char const * topic_name );
    ~dds_topic();

    eprosima::fastdds::dds::Topic * get() const { return _topic; }
    eprosima::fastdds::dds::Topic * operator->() const { return get(); }

    std::shared_ptr< dds_participant > const & get_participant() const { return _participant; }

    // NOTE: the following useful functions are already defined in Topic:
    //     get_name()        // name of the topic
    //     get_type_name()   // type of the topic
};


}  // namespace dds
}  // namespace librealsense