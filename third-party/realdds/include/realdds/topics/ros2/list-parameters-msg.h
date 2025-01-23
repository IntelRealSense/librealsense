// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>

#include <realdds/topics/ros2/rcl_interfaces/srv/ListParameters.h>
#include <realdds/topics/ros2/rcl_interfaces/msg/ListParametersResult.h>

#include <string>
#include <memory>
#include <vector>
#include <iosfwd>


namespace rcl_interfaces {
namespace srv {
class ListParameters_RequestPubSubType;
class ListParameters_ResponsePubSubType;
}  // namespace srv
}  // namespace rcl_interfaces


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {
namespace ros2 {


class list_parameters_request_msg
{
    rcl_interfaces::srv::ListParameters_Request _raw;

public:
    using type = rcl_interfaces::srv::ListParameters_RequestPubSubType;

    static const auto DEPTH_RECURSIVE = rcl_interfaces::srv::ListParameters_Request_Constants::DEPTH_RECURSIVE;

    // Relative depth from given prefixes
    // Use DEPTH_RECURSIVE (the default) to get the recursive parameters and prefixes for each prefix; unlimited depth
    uint64_t depth() const { return _raw.depth(); }
    void depth( uint64_t d ) { _raw.depth( d ); }

    // If the prefixes are empty, returns all parameters

    void clear() { _raw.prefixes().clear(); }
    std::vector< std::string > const & prefixes() const { return _raw.prefixes(); }
    void add( std::string const & prefix ) { _raw.prefixes().push_back( prefix ); }

    bool is_valid() const { return depth() != 0xBADF00D; }
    void invalidate() { depth( 0xBADF00D ); }


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

std::ostream & operator<<( std::ostream &, list_parameters_request_msg const & );


class list_parameters_response_msg
{
    rcl_interfaces::srv::ListParameters_Response _raw;

public:
    using type = rcl_interfaces::srv::ListParameters_ResponsePubSubType;

    std::vector< std::string > const & names() const { return _raw.result().names(); }
    std::vector< std::string > const & prefixes() const { return _raw.result().prefixes(); }
    bool is_valid() const { return true; }
    void clear() { _raw.result().names().clear(); }

    void add( std::string const & name ) { _raw.result().names().push_back( name ); }
    void add_prefix( std::string const & prefix ) { _raw.result().prefixes().push_back( prefix ); }


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
    static bool take_next( dds_topic_reader &,
                           list_parameters_response_msg * output,
                           dds_sample * optional_sample = nullptr );
};


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
