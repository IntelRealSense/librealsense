// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "platform-camera.h"
#include "ds/ds-timestamp.h"
#include "environment.h"
#include "stream.h"
#include "proc/color-formats-converter.h"
#include "backend.h"
#include "platform/platform-utils.h"
#include <src/fourcc.h>
#include <src/metadata-parser.h>


namespace librealsense {
namespace {


const std::map< uint32_t, rs2_format > platform_color_fourcc_to_rs2_format = {
    { rs_fourcc( 'Y', 'U', 'Y', '2' ), RS2_FORMAT_YUYV },
    { rs_fourcc( 'Y', 'U', 'Y', 'V' ), RS2_FORMAT_YUYV },
    { rs_fourcc( 'M', 'J', 'P', 'G' ), RS2_FORMAT_MJPEG },
};
const std::map< uint32_t, rs2_stream > platform_color_fourcc_to_rs2_stream = {
    { rs_fourcc( 'Y', 'U', 'Y', '2' ), RS2_STREAM_COLOR },
    { rs_fourcc( 'Y', 'U', 'Y', 'V' ), RS2_STREAM_COLOR },
    { rs_fourcc( 'M', 'J', 'P', 'G' ), RS2_STREAM_COLOR },
};


class platform_camera_sensor : public synthetic_sensor
{
public:
    platform_camera_sensor( device * owner, std::shared_ptr< uvc_sensor > uvc_sensor )
        : synthetic_sensor(
            "RGB Camera", uvc_sensor, owner, platform_color_fourcc_to_rs2_format, platform_color_fourcc_to_rs2_stream )
        , _default_stream( new stream( RS2_STREAM_COLOR ) )
    {
    }

    stream_profiles init_stream_profiles() override
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();

        auto results = synthetic_sensor::init_stream_profiles();

        for( auto && p : results )
        {
            // Register stream types
            assign_stream( _default_stream, p );
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics( *_default_stream, *p );
        }

        return results;
    }


private:
    std::shared_ptr< stream_interface > _default_stream;
};


}  // namespace


platform_camera::platform_camera( std::shared_ptr< const device_info > const & dev_info,
                                  const std::vector< platform::uvc_device_info > & uvc_infos,
                                  bool register_device_notifications )
    : device( dev_info, register_device_notifications )
    , backend_device( dev_info, register_device_notifications )
{
    std::vector< std::shared_ptr< platform::uvc_device > > devs;
    auto backend = get_backend();
    for( auto & info : uvc_infos )
        devs.push_back( backend->create_uvc_device( info ) );

    std::unique_ptr< frame_timestamp_reader > host_timestamp_reader_backup( new ds_timestamp_reader() );
    auto raw_color_ep = std::make_shared< uvc_sensor >(
        "Raw RGB Camera",
        std::make_shared< platform::multi_pins_uvc_device >( devs ),
        std::unique_ptr< frame_timestamp_reader >(
            new ds_timestamp_reader_from_metadata( std::move( host_timestamp_reader_backup ) ) ),
        this );
    auto color_ep = std::make_shared< platform_camera_sensor >( this, raw_color_ep );
    add_sensor( color_ep );

    register_info( RS2_CAMERA_INFO_NAME, "Platform Camera" );
    std::string pid_str( rsutils::string::from()
                         << std::setfill( '0' ) << std::setw( 4 ) << std::hex << uvc_infos.front().pid );
    std::transform( pid_str.begin(), pid_str.end(), pid_str.begin(), ::toupper );

    using namespace platform;
    auto usb_mode = raw_color_ep->get_usb_specification();
    std::string usb_type_str( "USB" );
    if( usb_spec_names.count( usb_mode ) && ( usb_undefined != usb_mode ) )
        usb_type_str = usb_spec_names.at( usb_mode );

    register_info( RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str );
    register_info( RS2_CAMERA_INFO_SERIAL_NUMBER, uvc_infos.front().unique_id );
    register_info( RS2_CAMERA_INFO_PHYSICAL_PORT, uvc_infos.front().device_path );
    register_info( RS2_CAMERA_INFO_PRODUCT_ID, pid_str );

    color_ep->register_processing_block(
        processing_block_factory::create_pbf_vector< yuy2_converter >( RS2_FORMAT_YUYV,
                                                                       map_supported_color_formats( RS2_FORMAT_YUYV ),
                                                                       RS2_STREAM_COLOR ) );
    color_ep->register_processing_block( { { RS2_FORMAT_MJPEG } },
                                         { { RS2_FORMAT_RGB8, RS2_STREAM_COLOR } },
                                         []() { return std::make_shared< mjpeg_converter >( RS2_FORMAT_RGB8 ); } );
    color_ep->register_processing_block(
        processing_block_factory::create_id_pbf( RS2_FORMAT_MJPEG, RS2_STREAM_COLOR ) );

    // Timestamps are given in units set by device which may vary among the OEM vendors.
    // For consistent (msec) measurements use "time of arrival" metadata attribute
    color_ep->register_metadata( RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                                 make_uvc_header_parser( &platform::uvc_header::timestamp ) );

    color_ep->try_register_pu( RS2_OPTION_BACKLIGHT_COMPENSATION );
    color_ep->try_register_pu( RS2_OPTION_BRIGHTNESS );
    color_ep->try_register_pu( RS2_OPTION_CONTRAST );
    color_ep->try_register_pu( RS2_OPTION_EXPOSURE );
    color_ep->try_register_pu( RS2_OPTION_GAMMA );
    color_ep->try_register_pu( RS2_OPTION_HUE );
    color_ep->try_register_pu( RS2_OPTION_SATURATION );
    color_ep->try_register_pu( RS2_OPTION_SHARPNESS );
    color_ep->try_register_pu( RS2_OPTION_WHITE_BALANCE );
    color_ep->try_register_pu( RS2_OPTION_ENABLE_AUTO_EXPOSURE );
    color_ep->try_register_pu( RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE );
}


std::vector< tagged_profile > platform_camera::get_profiles_tags() const
{
    std::vector< tagged_profile > markers;
    markers.push_back( { RS2_STREAM_COLOR,
                         -1,
                         640,
                         480,
                         RS2_FORMAT_RGB8,
                         30,
                         profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    return markers;
}


/*static*/ std::vector< std::shared_ptr< platform_camera_info > >
platform_camera_info::pick_uvc_devices( const std::shared_ptr< context > & ctx,
                                        const std::vector< platform::uvc_device_info > & uvc_devices )
{
    std::vector< std::shared_ptr< platform_camera_info > > list;
    auto groups = group_devices_by_unique_id( uvc_devices );

    for( auto && g : groups )
    {
        if( g.front().vid != VID_INTEL_CAMERA )
            list.push_back( std::make_shared< platform_camera_info >( ctx, std::move( g ) ) );
    }
    return list;
}


}  // namespace librealsense
