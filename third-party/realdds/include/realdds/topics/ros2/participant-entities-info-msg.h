// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>

#include <realdds/topics/ros2/rmw_dds_common/msg/ParticipantEntitiesInfo.h>

#include <string>
#include <memory>
#include <vector>


namespace rmw_dds_common {
namespace msg {
class ParticipantEntitiesInfoPubSubType;
}  // namespace srv
}  // namespace rmw_dds_common


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {
namespace ros2 {


class node_entities_info : public rmw_dds_common::msg::NodeEntitiesInfo
{
    using super = rmw_dds_common::msg::NodeEntitiesInfo;

public:
    node_entities_info( std::string const & ns, std::string const & name )
    {
        if( ! ns.empty() )
            node_namespace( ns );
        node_name( name );
    }

    void add_writer( dds_guid const & );
    void add_reader( dds_guid const & );
};


// This message is sent by each ROS2 participant over the 'ros_discovery_info' topic, to broadcast all nodes and their
// entities. Without this message, ROS2 will not recognize nodes!
//
class participant_entities_info_msg
{
    rmw_dds_common::msg::ParticipantEntitiesInfo _raw;

public:
    using type = rmw_dds_common::msg::ParticipantEntitiesInfoPubSubType;

    dds_guid gid() const;
    void set_gid( dds_guid const & );

    using node = node_entities_info;

    void clear() { _raw.node_entities_info_seq().clear(); }
    std::vector< rmw_dds_common::msg::NodeEntitiesInfo > const & nodes() const { return _raw.node_entities_info_seq(); }
    void add( node const & new_node ) { _raw.node_entities_info_seq().push_back( new_node ); }

    bool is_valid() const { return ( _raw.gid() != rmw_dds_common::msg::Gid() ); }
    void invalidate() { _raw.gid( rmw_dds_common::msg::Gid() ); }


    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );


    // Returns some unique (to the writer) identifier for the sample that was sent, or 0 if unsuccessful
    dds_sequence_number write_to( dds_topic_writer & );


    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    //Note - copies the data.
    //TODO - add an API for a function that loans the data and enables the user to free it later.
    static bool take_next( dds_topic_reader &,
                           participant_entities_info_msg * output,
                           dds_sample * optional_sample = nullptr );
};


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
