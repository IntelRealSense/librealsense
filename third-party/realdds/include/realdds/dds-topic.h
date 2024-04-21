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


namespace realdds {


class dds_participant;


// RAII for DDS/ROS topics:
//      - The length of the DDS topic [name] must not exceed 256 characters
//      - ROS2 topic names:
//          - http://design.ros2.org/articles/topic_and_service_names.html#dds-topic-names
//          - prefixed with "rt/" (note: can be overriden with avoid_ros_namespace_conventions)
//          - start with [a-zA-z]; continue with [a-zA-z0-9_/]; see additional rules in above link
//          - i.e., NO SPACES
//
class dds_topic
{
    std::shared_ptr< dds_participant > const _participant;
    eprosima::fastdds::dds::Topic * _topic;

public:
    dds_topic( std::shared_ptr< dds_participant > const & participant,
               eprosima::fastdds::dds::TypeSupport const & topic_type,
               char const * topic_name );
    ~dds_topic();

#if 0  // this doesn't work in gcc...
    // Helper function: given the class of the type you want (e.g., device_info), and given that this class
    // defines 'type' and 'TOPIC_NAME', this can make things a little more readable:
    //
    template< class type >
    static std::shared_ptr< dds_topic > of( std::shared_ptr< dds_participant > const & participant )
    {
        return std::make_shared< dds_topic >( participant,
                                              eprosima::fastdds::dds::TypeSupport( new type::type ),
                                              type::TOPIC_NAME );
    }
#endif

    eprosima::fastdds::dds::Topic * get() const { return _topic; }
    eprosima::fastdds::dds::Topic * operator->() const { return get(); }

    std::shared_ptr< dds_participant > const & get_participant() const { return _participant; }

    // NOTE: the following useful functions are already defined in Topic:
    //     get_name()        // name of the topic
    //     get_type_name()   // type of the topic
};


}  // namespace realdds
