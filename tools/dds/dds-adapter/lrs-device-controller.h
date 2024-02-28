// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <realdds/dds-stream-sensor-bridge.h>
#include <realdds/dds-stream-profile.h>

#include <rsutils/json-fwd.h>
#include <map>
#include <vector>

namespace rs2 {
    class frame;
}

namespace realdds {

class dds_device_server;
class dds_stream_server;
class dds_option;

} // namespace realdds


namespace tools {

// This class is in charge of handling a RS device: streaming, control..
class lrs_device_controller : public std::enable_shared_from_this< lrs_device_controller >
{
public:
    lrs_device_controller( rs2::device dev, std::shared_ptr< realdds::dds_device_server > dds_device_server );
    ~lrs_device_controller();

    void set_option( const std::shared_ptr< realdds::dds_option > & option, float new_value );
    rsutils::json query_option( const std::shared_ptr< realdds::dds_option > & option );

    bool is_recovery() const;

private:
    std::vector< std::shared_ptr< realdds::dds_stream_server > > get_supported_streams();

    void publish_frame_metadata( const rs2::frame & f, realdds::dds_time const & );

    bool on_control( std::string const & id, rsutils::json const & control, rsutils::json & reply );
    bool on_hardware_reset( rsutils::json const &, rsutils::json & );
    bool on_hwm( rsutils::json const &, rsutils::json & );
    bool on_dfu_start( rsutils::json const &, rsutils::json & );
    bool on_dfu_apply( rsutils::json const &, rsutils::json & );
    bool on_open_streams( rsutils::json const &, rsutils::json & );

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
};  // class lrs_device_controller

}  // namespace tools
