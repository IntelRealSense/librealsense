// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>

#include <realdds/topics/ros2/rcl_interfaces/srv/SetParameters.h>
#include <realdds/topics/ros2/rcl_interfaces/msg/Parameter.h>
#include <realdds/topics/ros2/rcl_interfaces/msg/SetParametersResult.h>

#include <string>
#include <memory>
#include <vector>


namespace rcl_interfaces {
namespace srv {
class SetParameters_RequestPubSubType;
class SetParameters_ResponsePubSubType;
}  // namespace srv
}  // namespace rcl_interfaces


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {
namespace ros2 {


class set_parameters_request_msg
{
    rcl_interfaces::srv::SetParameters_Request _raw;

public:
    using type = rcl_interfaces::srv::SetParameters_RequestPubSubType;

    using parameter_type = rcl_interfaces::msg::Parameter;

    void clear() { _raw.parameters().clear(); }
    std::vector< parameter_type > const & parameters() const { return _raw.parameters(); }
    void add( parameter_type const & parameter ) { _raw.parameters().push_back( parameter ); }

    bool is_valid() const { return ! parameters().empty(); }
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


class set_parameters_response_msg
{
    rcl_interfaces::srv::SetParameters_Response _raw;

public:
    using type = rcl_interfaces::srv::SetParameters_ResponsePubSubType;

    using result_type = rcl_interfaces::msg::SetParametersResult;

    std::vector< result_type > const & results() const { return _raw.results(); }
    void add( result_type const & result ) { _raw.results().push_back( result ); }
    bool is_valid() const { return ! results().empty(); }
    void invalidate() { _raw.results().clear(); }


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
