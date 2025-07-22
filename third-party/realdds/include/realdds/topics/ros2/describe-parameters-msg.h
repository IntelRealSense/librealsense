// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>

#include <realdds/topics/ros2/rcl_interfaces/srv/DescribeParameters.h>
#include <realdds/topics/ros2/rcl_interfaces/msg/ParameterDescriptor.h>

#include <string>
#include <memory>
#include <vector>


namespace rcl_interfaces {
namespace srv {
class DescribeParameters_RequestPubSubType;
class DescribeParameters_ResponsePubSubType;
}  // namespace srv
}  // namespace rcl_interfaces


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {
namespace ros2 {


class describe_parameters_request_msg
{
    rcl_interfaces::srv::DescribeParameters_Request _raw;

public:
    using type = rcl_interfaces::srv::DescribeParameters_RequestPubSubType;

    void clear() { _raw.names().clear(); }
    std::vector< std::string > const & names() const { return _raw.names(); }
    void add( std::string const & name ) { _raw.names().push_back( name ); }

    bool is_valid() const { return ! names().empty(); }
    void invalidate() { clear(); }


    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      std::string const & topic_name )
    {
        return create_topic( participant, topic_name.c_str() );
    }

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    //Note - copies the data.
    //TODO - add an API for a function that loans the data and enables the user to free it later.
    bool take_next( dds_topic_reader &, dds_sample * optional_sample = nullptr );
};


class describe_parameters_response_msg
{
    rcl_interfaces::srv::DescribeParameters_Response _raw;

public:
    using type = rcl_interfaces::srv::DescribeParameters_ResponsePubSubType;

    using descriptor_type = rcl_interfaces::msg::ParameterDescriptor;

    std::vector< descriptor_type > const & descriptors() const { return _raw.descriptors(); }
    bool is_valid() const { return ! descriptors().empty(); }
    void invalidate() { _raw.descriptors().clear(); }

    void add( descriptor_type const & descriptor )
    {
        _raw.descriptors().push_back( descriptor );
    }


    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      std::string const & topic_name )
    {
        return create_topic( participant, topic_name.c_str() );
    }

    // Sends out the response to the given writer
    // The request sample is needed for ROS2 request-response mechanism to connect the two
    // Returns a unique (to the writer) identifier for the sample that was sent, or 0 if unsuccessful
    dds_sequence_number respond_to( dds_sample const & request_sample, dds_topic_writer & ) const;

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    //Note - copies the data.
    //TODO - add an API for a function that loans the data and enables the user to free it later.
    bool take_next( dds_topic_reader &, dds_sample * optional_sample = nullptr );
};


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
