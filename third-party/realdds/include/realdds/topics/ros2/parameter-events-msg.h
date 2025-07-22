// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>

#include <realdds/topics/ros2/rcl_interfaces/msg/ParameterEvent.h>

#include <fastdds/rtps/common/Time_t.h>
#include <string>
#include <memory>
#include <vector>


namespace rcl_interfaces {
namespace msg {
class ParameterEventPubSubType;
}  // namespace srv
}  // namespace rcl_interfaces


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {
namespace ros2 {


// A ROS2 notification, on /parameter_events, when some parameter is added/deleted/changed
//
class parameter_events_msg
{
    rcl_interfaces::msg::ParameterEvent _raw;

public:
    using type = rcl_interfaces::msg::ParameterEventPubSubType;


    void set_timestamp( dds_time const & t )
    {
        _raw.stamp().sec( t.seconds );
        _raw.stamp().nanosec( t.nanosec );
    }
    dds_time timestamp() const { return { _raw.stamp().sec(), _raw.stamp().nanosec() }; }

    void set_node_name( std::string const & node_name ) { _raw.node( node_name ); }
    std::string const & node_name() const { return _raw.node(); }


    using param_type = rcl_interfaces::msg::Parameter;

    std::vector< param_type > const & new_params() const { return _raw.new_parameters(); }
    void add_new_param( param_type && param ) { _raw.new_parameters().push_back( std::move( param ) ); }
    std::vector< param_type > const & changed_params() const { return _raw.changed_parameters(); }
    void add_changed_param( param_type && param ) { _raw.changed_parameters().push_back( std::move( param ) ); }
    std::vector< param_type > const & deleted_params() const { return _raw.deleted_parameters(); }
    void add_deleted_param( param_type && param ) { _raw.deleted_parameters().push_back( std::move( param ) ); }


    bool is_valid() const { return ! node_name().empty(); }
    void invalidate() { _raw.node().clear(); }


    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );


    // Returns some unique (to the writer) identifier for the sample that was sent, or 0 if unsuccessful
    dds_sequence_number write_to( dds_topic_writer & ) const;


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
