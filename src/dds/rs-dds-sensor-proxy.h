// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


#include "sid_index.h"
#include <src/software-device.h>
#include <src/proc/formats-converter.h>

#include <realdds/dds-metadata-syncer.h>

#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <map>


namespace realdds {
class dds_device;
class dds_stream;
class dds_option;
class dds_video_stream_profile;
class dds_motion_stream_profile;
namespace topics {
class image_msg;
class imu_msg;
}  // namespace topics
}  // namespace realdds


namespace librealsense {


class dds_device_proxy;


class dds_sensor_proxy : public software_sensor
{
    std::shared_ptr< realdds::dds_device > const _dev;
    std::string const _name;
    bool const _md_enabled;

    typedef realdds::dds_metadata_syncer syncer_type;
    static void frame_releaser( syncer_type::frame_type * f ) { static_cast< frame * >( f )->release(); }

    struct streaming_impl
    {
        syncer_type syncer;
        std::atomic< unsigned long long > last_frame_number{ 0 };
    };

    std::map< sid_index, std::shared_ptr< realdds::dds_stream > > _streams;
    std::map< std::string, streaming_impl > _streaming_by_name;

    formats_converter _formats_converter;
    // DDS profiles are stored in _streams, _raw_rs_profiles stores librealsense representation of them,
    // software_device::_profiles stores librealsense profiles after conversion from raw to a user friendly format.
    stream_profiles _raw_rs_profiles;

public:
    dds_sensor_proxy( std::string const & sensor_name,
                      software_device * owner,
                      std::shared_ptr< realdds::dds_device > const & dev );

    const std::string & get_name() const { return _name; }

    void add_dds_stream( sid_index sidx, std::shared_ptr< realdds::dds_stream > const & stream );
    std::shared_ptr<stream_profile_interface> add_video_stream( rs2_video_stream video_stream, bool is_default ) override;
    std::shared_ptr<stream_profile_interface> add_motion_stream( rs2_motion_stream motion_stream, bool is_default ) override;

    void open( const stream_profiles & profiles ) override;
    void start( frame_callback_ptr callback ) override;
    void stop();

    void add_option( std::shared_ptr< realdds::dds_option > option );
    void set_option( const std::string & name, float value ) const;
    float query_option( const std::string & name ) const;

    void add_processing_block( std::string filter_name );
    bool processing_block_exists( processing_blocks const & blocks, std::string const & block_name ) const;
    void create_processing_block( std::string & filter_name );

    const std::map< sid_index, std::shared_ptr< realdds::dds_stream > > & streams() const { return _streams; }

private:
    void register_basic_converters();
    stream_profiles init_stream_profiles() override;

    std::shared_ptr< realdds::dds_video_stream_profile >
    find_profile( sid_index sidx, realdds::dds_video_stream_profile const & profile ) const;

    std::shared_ptr< realdds::dds_motion_stream_profile >
    find_profile( sid_index sidx, realdds::dds_motion_stream_profile const & profile ) const;

    void handle_video_data( realdds::topics::image_msg && dds_frame,
                            const std::shared_ptr< stream_profile_interface > &,
                            streaming_impl & streaming );
    void handle_motion_data( realdds::topics::imu_msg &&,
                             const std::shared_ptr< stream_profile_interface > &,
                             streaming_impl & );
    void handle_new_metadata( std::string const & stream_name, nlohmann::json && metadata );

    void add_frame_metadata( frame * const, nlohmann::json && metadata, streaming_impl & );

    friend class dds_device_proxy;  // Currently calls handle_new_metadata
};


}  // namespace librealsense
