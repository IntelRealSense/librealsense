// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "rs-dds-internal-data.h"

#include <librealsense2/h/rs_sensor.h>
#include <proc/processing-blocks-factory.h>
#include <proc/color-formats-converter.h>
#include <proc/depth-formats-converter.h>
#include <proc/y8i-to-y8y8.h>
#include <proc/y12i-to-y16y16.h>
#include <proc/y12i-to-y16y16-mipi.h>

namespace librealsense {

static const std::string RS405_PID = "0B5B";
static const std::string RS415_PID = "0AD3";
static const std::string RS435_RGB_PID = "0B07";
static const std::string RS435I_PID = "0B3A";
static const std::string RS455_PID = "0B5C";
static const std::string RS457_PID = "ABCD";


std::vector< rs2_format > target_formats( rs2_format source_format )
{
    // Mapping from source color format to all of the compatible target color formats.
    std::vector< rs2_format > formats = { RS2_FORMAT_RGB8, RS2_FORMAT_RGBA8, RS2_FORMAT_BGR8, RS2_FORMAT_BGRA8 };

    switch( source_format )
    {
    case RS2_FORMAT_YUYV:
        formats.push_back( RS2_FORMAT_YUYV );
        formats.push_back( RS2_FORMAT_Y16 );
        formats.push_back( RS2_FORMAT_Y8 );
        break;
    case RS2_FORMAT_UYVY:
        formats.push_back( RS2_FORMAT_UYVY );
        break;
    default:
        throw std::runtime_error( "Format is not supported for mapping" );
    }

    return formats;
}

std::vector< tagged_profile > dds_rs_internal_data::get_profiles_tags( std::string product_id,
                                                                       std::string product_line )
{
    std::vector<tagged_profile> tags;

    if( product_id == RS405_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 848, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
    }
    else if( product_id == RS415_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 1280, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 1280, 720, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
    }
    else if( product_id == RS435_RGB_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
    }
    else if( product_id == RS435I_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
        tags.push_back( { RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 100, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    }
    else if( product_id == RS455_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
        tags.push_back( { RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 100, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    }
    else if( product_id == RS457_PID )
    {
        tags.push_back( { RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    }
    else if( product_line == "D400" )
    {
        tags.push_back( { RS2_STREAM_DEPTH, -1, 1280, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, 1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, 2, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
    }
    else if( product_line == "L500" )
    {
        tags.push_back( { RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_CONFIDENCE, -1, 640, 480, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
        tags.push_back( { RS2_STREAM_DEPTH, -1, -1, -1, RS2_FORMAT_FG, -1, profile_tag::PROFILE_TAG_DEBUG } );
        tags.push_back( { RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    }
    else if( product_line == "SR300" )
    {
        tags.push_back( { RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
        tags.push_back( { RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET } );
        tags.push_back( { RS2_STREAM_COLOR, -1, 1920, 1080, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT } );
    }
    else
        throw std::runtime_error( "Unsupported product id or product line" );

    return tags;
}


std::vector< processing_block_factory > dds_rs_internal_data::get_profile_converters( std::string product_id,
                                                                                      std::string product_line )
{
    std::vector< processing_block_factory > factories;
    std::vector< processing_block_factory > tmp;

    if( product_line == "D400" )
    {
        if( product_id == RS457_PID )
        {
            tmp = processing_block_factory::create_pbf_vector< uyvy_converter >( RS2_FORMAT_UYVY,
                                                                                 target_formats( RS2_FORMAT_UYVY ),
                                                                                 RS2_STREAM_COLOR );
            for( auto & it : tmp )
                factories.push_back( std::move( it ) );
            tmp = processing_block_factory::create_pbf_vector< uyvy_converter >( RS2_FORMAT_YUYV,
                                                                                 target_formats( RS2_FORMAT_YUYV ),
                                                                                 RS2_STREAM_COLOR );
            for( auto & it : tmp )
                factories.push_back( std::move( it ) );
            factories.push_back( { { { RS2_FORMAT_Y12I } },
                                   { { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1 },
                                     { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 2 } },
                                   []() { return std::make_shared< y12i_to_y16y16_mipi >(); } } );
        }
        else
        {
            tmp = processing_block_factory::create_pbf_vector< yuy2_converter >( RS2_FORMAT_YUYV,
                                                                                 target_formats( RS2_FORMAT_YUYV ),
                                                                                 RS2_STREAM_COLOR );
            for( auto & it : tmp )
                factories.push_back( std::move( it ) );
            factories.push_back( processing_block_factory::create_id_pbf( RS2_FORMAT_RAW16, RS2_STREAM_COLOR ) );
            factories.push_back( { { { RS2_FORMAT_Y12I } },
                                   { { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1 },
                                     { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 2 } },
                                   []() { return std::make_shared< y12i_to_y16y16 >(); } } );
        }
        tmp = processing_block_factory::create_pbf_vector< uyvy_converter >(
                               RS2_FORMAT_UYVY,
                               target_formats( RS2_FORMAT_UYVY ),
                               RS2_STREAM_INFRARED );
        for( auto & it : tmp )
            factories.push_back( std::move( it ) );
        factories.push_back( processing_block_factory::create_id_pbf( RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 ) );
        factories.push_back( processing_block_factory::create_id_pbf( RS2_FORMAT_Z16, RS2_STREAM_DEPTH ) );
        factories.push_back( { { { RS2_FORMAT_W10 } },
                               { { RS2_FORMAT_RAW10, RS2_STREAM_INFRARED, 1 } },
                               []() { return std::make_shared< w10_converter >( RS2_FORMAT_RAW10 ); } } );
        factories.push_back( { { { RS2_FORMAT_W10 } },
                               { { RS2_FORMAT_Y10BPACK, RS2_STREAM_INFRARED, 1 } },
                               []() { return std::make_shared< w10_converter >( RS2_FORMAT_Y10BPACK ); } } );
        factories.push_back( { { { RS2_FORMAT_Y8I } },
                               { { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 },
                                 { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 2 } },
                               []() { return std::make_shared< y8i_to_y8y8 >(); } } );

        // Motion convertion is done on the camera side.
        // Intrinsics table is being read from the camera flash and it is too much to set here, this file is ment as a
        // temporary solution, the camera should send all this data in a designated topic.
        factories.push_back( { { { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL } },
                               { { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL } },
                               []() { return std::make_shared< identity_processing_block >(); } } );
        factories.push_back( { { { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO } },
                               { { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO } },
                               []() { return std::make_shared< identity_processing_block >(); } } );
    }
    //else if( product_line == "L500" )
    //{
    //    processing_block_factory::create_pbf_vector< yuy2_converter >( RS2_FORMAT_YUYV, map_supported_color_formats( RS2_FORMAT_YUYV ), RS2_STREAM_COLOR );
    //    (
    //        { {RS2_FORMAT_Z16}, {RS2_FORMAT_Y8} },
    //        { {RS2_FORMAT_Z16, RS2_STREAM_DEPTH, 0, 0, 0, 0, &rotate_resolution} },
    //        [=]() {
    //            auto z16rot = std::make_shared<rotation_transform>( RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME );
    //            auto y8rot = std::make_shared<rotation_transform>( RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME );
    //            auto sync = std::make_shared<syncer_process_unit>( nullptr, false ); // disable logging on this internal syncer

    //            auto cpb = std::make_shared<composite_processing_block>();
    //            cpb->add( z16rot );
    //            cpb->add( y8rot );
    //            cpb->add( sync );
    //            cpb->add( std::make_shared< filtering_processing_block >( RS2_STREAM_DEPTH ) );
    //            return cpb;
    //        }
    //    );
    //    (
    //        { {RS2_FORMAT_Z16}, {RS2_FORMAT_Y8}, {RS2_FORMAT_RAW8} },
    //        {
    //            {RS2_FORMAT_Z16, RS2_STREAM_DEPTH, 0, 0, 0, 0, &rotate_resolution},
    //            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 0, 0, 0, &l500_confidence_resolution}
    //        },
    //        [=]() {
    //            auto z16rot = std::make_shared<rotation_transform>( RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME );
    //            auto y8rot = std::make_shared<rotation_transform>( RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME );
    //            auto conf = std::make_shared<confidence_rotation_transform>();
    //            auto sync = std::make_shared<syncer_process_unit>( nullptr, false ); // disable logging on this internal syncer

    //            auto cpb = std::make_shared<composite_processing_block>();
    //            cpb->add( z16rot );
    //            cpb->add( y8rot );
    //            cpb->add( conf );
    //            cpb->add( sync );
    //            cpb->add( std::shared_ptr< filtering_processing_block >(
    //                new filtering_processing_block { RS2_STREAM_DEPTH, RS2_STREAM_CONFIDENCE } ) );
    //            return cpb;
    //        }
    //    );
    //    (
    //        { {RS2_FORMAT_Y8} },
    //        { {RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 0, 0, 0, 0, &rotate_resolution} },
    //        []() { return std::make_shared<rotation_transform>( RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME ); }
    //    );
    //    (
    //        { {RS2_FORMAT_RAW8} },
    //        { {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 0, 0, 0, &l500_confidence_resolution} },
    //        []() { return std::make_shared<confidence_rotation_transform>(); }
    //    );
    //    ( processing_block_factory::create_id_pbf( RS2_FORMAT_FG, RS2_STREAM_DEPTH ) );
    //    (
    //        { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL} },
    //        { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL} },
    //        [&, mm_correct_opt]() { return std::make_shared<acceleration_transform>( _mm_calib, mm_correct_opt ); }
    //    );
    //    (
    //        { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO} },
    //        { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO} },
    //        [&, mm_correct_opt]() { return std::make_shared<gyroscope_transform>( _mm_calib, mm_correct_opt ); }
    //    );
    //}
    //else if( product_line == "SR300" )
    //{
    //    processing_block_factory::create_pbf_vector<uyvy_converter>( RS2_FORMAT_UYVY, map_supported_color_formats( RS2_FORMAT_UYVY ), RS2_STREAM_COLOR );
    //    processing_block_factory::create_pbf_vector<yuy2_converter>( RS2_FORMAT_YUYV, map_supported_color_formats( RS2_FORMAT_YUYV ), RS2_STREAM_COLOR );
    //    ( { { RS2_FORMAT_INVI } }, { { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 } }, []() {return std::make_shared<invi_converter>( RS2_FORMAT_Y8 ); } );
    //    ( { { RS2_FORMAT_INVI } },  { { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1 } }, []() {return std::make_shared<invi_converter>( RS2_FORMAT_Y16 ); } );
    //    ( { { RS2_FORMAT_INZI } }, { { RS2_FORMAT_Z16, RS2_STREAM_DEPTH }, { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 } }, []() {return std::make_shared<inzi_converter>( RS2_FORMAT_Y8 ); } );
    //    ( { { RS2_FORMAT_INZI } }, { { RS2_FORMAT_Z16, RS2_STREAM_DEPTH }, { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1 } }, []() {return std::make_shared<inzi_converter>( RS2_FORMAT_Y16 ); } );
    //    processing_block_factory::create_id_pbf( RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 );
    //    processing_block_factory::create_id_pbf( RS2_FORMAT_Z16, RS2_STREAM_DEPTH );
    //}
    else
        throw std::runtime_error( "Unsupported product id or product line" );

    return factories;
}

}  // namespace librealsense
