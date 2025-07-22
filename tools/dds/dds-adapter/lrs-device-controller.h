// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <realdds/dds-stream-sensor-bridge.h>
#include <realdds/dds-stream-profile.h>

#include <rsutils/json-fwd.h>
#include <rsutils/concurrency/concurrency.h>
#include <map>
#include <vector>

namespace rs2 {
    class frame;
}

namespace realdds {

class dds_device_server;
class dds_stream_server;
class dds_option;
class dds_topic_reader;
class dds_topic_writer;

namespace topics {
namespace ros2 {

class node_entities_info;

}  // namespace ros2
}  // namespace topics
} // namespace realdds


namespace rmw_dds_common {
namespace msg {
class NodeEntitiesInfo;
}  // namespace msg
}  // namespace rmw_dds_common


namespace tools {

// This class is in charge of handling a RS device: streaming, control..
class lrs_device_controller : public std::enable_shared_from_this< lrs_device_controller >
{
public:
    lrs_device_controller( rs2::device dev, std::shared_ptr< realdds::dds_device_server > dds_device_server );
    ~lrs_device_controller();

    void set_option( const std::shared_ptr< realdds::dds_option > & option, rsutils::json const & new_value );
    rsutils::json query_option( const std::shared_ptr< realdds::dds_option > & option );

    bool is_recovery() const;

    void initialize_ros2_node_entities( std::string const & node_name,
                                        std::shared_ptr< realdds::dds_topic_writer > parameter_events_writer );
    void fill_ros2_node_entities( realdds::topics::ros2::node_entities_info & ) const;

private:
    std::vector< std::shared_ptr< realdds::dds_stream_server > > get_supported_streams();
    bool update_stream_trinsics( rsutils::json * p_changes = nullptr );

    void publish_frame_metadata( const rs2::frame & f, realdds::dds_time const & );

    bool on_control( std::string const & id, rsutils::json const & control, rsutils::json & reply );
    bool on_hardware_reset( rsutils::json const &, rsutils::json & );
    bool on_hwm( rsutils::json const &, rsutils::json & );
    bool on_dfu_start( rsutils::json const &, rsutils::json & );
    bool on_dfu_apply( rsutils::json const &, rsutils::json & );
    bool on_open_streams( rsutils::json const &, rsutils::json & );

    void on_get_params_request();
    void on_set_params_request();
    void on_list_params_request();
    void on_describe_params_request();

    void override_default_profiles( const std::map< std::string, realdds::dds_stream_profiles > & stream_name_to_profiles,
                                    std::map< std::string, size_t > & stream_name_to_default_profile ) const;
    size_t get_index_of_profile( const realdds::dds_stream_profiles & profiles,
                                 const realdds::dds_video_stream_profile & profile ) const;
    size_t get_index_of_profile( const realdds::dds_stream_profiles & profiles,
                                 const realdds::dds_motion_stream_profile & profile ) const;

    std::shared_ptr< realdds::dds_stream_server > frame_to_streaming_server( rs2::frame const &,
                                                                             rs2::stream_profile * = nullptr ) const;


    rs2::device _rs_dev;
    std::map< std::string, rs2::sensor > _rs_sensors;
    std::string _device_sn;
    realdds::dds_stream_sensor_bridge _bridge;

    struct dfu_support;
    std::shared_ptr< dfu_support > _dfu;

    std::map< std::string, std::shared_ptr< realdds::dds_stream_server > > _stream_name_to_server;

    std::vector< rs2::stream_profile > get_rs2_profiles( realdds::dds_stream_profiles const & dds_profiles ) const;

    std::shared_ptr< realdds::dds_device_server > _dds_device_server;
    bool _md_enabled;

    dispatcher _control_dispatcher;

    std::string _ros2_node_name;
    std::shared_ptr< realdds::dds_topic_reader > _get_params_reader;
    std::shared_ptr< realdds::dds_topic_writer > _get_params_writer;
    std::shared_ptr< realdds::dds_topic_reader > _set_params_reader;
    std::shared_ptr< realdds::dds_topic_writer > _set_params_writer;
    std::shared_ptr< realdds::dds_topic_reader > _list_params_reader;
    std::shared_ptr< realdds::dds_topic_writer > _list_params_writer;
    std::shared_ptr< realdds::dds_topic_reader > _describe_params_reader;
    std::shared_ptr< realdds::dds_topic_writer > _describe_params_writer;
    std::shared_ptr< realdds::dds_topic_writer > _parameter_events_writer;
};  // class lrs_device_controller

}  // namespace tools
