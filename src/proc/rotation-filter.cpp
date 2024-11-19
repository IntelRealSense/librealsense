// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <librealsense2/hpp/rs_sensor.hpp>
#include "option.h"
#include "stream.h"
#include "core/video.h"
#include "proc/rotation-filter.h"

namespace librealsense {

    const int rotation_min_val = -90;
    const int rotation_max_val = 180;  
    const int rotation_default_val = 0;
    const int rotation_step = 90; 


    rotation_filter::rotation_filter()
        : stream_filter_processing_block( "Rotation Filter" )
        , _streams_to_rotate()
        , _control_val( rotation_default_val )
        , _real_width( 0 )
        , _real_height( 0 )
        , _rotated_width( 0 )
        , _rotated_height( 0 )
        , _value( 0 )
    {
        auto rotation_control = std::make_shared< ptr_option< int > >( rotation_min_val,
                                                                       rotation_max_val,
                                                                       rotation_step,
                                                                       rotation_default_val,
                                                                       &_control_val,
                                                                       "Rotation angle" );

        auto weak_rotation_control = std::weak_ptr< ptr_option< int > >( rotation_control );
        rotation_control->on_set(
            [this, weak_rotation_control]( float val )
            {
                auto strong_rotation_control = weak_rotation_control.lock();
                if( ! strong_rotation_control )
                    return;

                std::lock_guard< std::mutex > lock( _mutex );

                if( ! strong_rotation_control->is_valid( val ) )
                    throw invalid_value_exception( rsutils::string::from()
                                                   << "Unsupported rotation scale " << val << " is out of range." );

                _value = val;
            } );

        register_option( RS2_OPTION_ROTATION, rotation_control ); 
    }

    rs2::frame rotation_filter::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        if( _value == rotation_default_val )
            return f;

        rs2::stream_profile profile = f.get_profile();
        rs2_stream type = profile.stream_type();
        rs2_extension tgt_type;
        if( type == RS2_STREAM_INFRARED )
        {
            tgt_type = RS2_EXTENSION_VIDEO_FRAME;
        }
        else if( f.is< rs2::disparity_frame >() )
        {
            tgt_type = RS2_EXTENSION_DISPARITY_FRAME;
        }
        else if( f.is< rs2::depth_frame >() )
        {
            tgt_type = RS2_EXTENSION_DEPTH_FRAME;
        }
        else
        {
            return f;
        }

        auto src = f.as< rs2::video_frame >();
        _target_stream_profile = profile;

        if( _value == 90 || _value == -90 )
        {
            _rotated_width = src.get_height();
            _rotated_height = src.get_width();
        }
        else if( _value == 180 )
        {
            _rotated_width = src.get_width();
            _rotated_height = src.get_height();
        }
        auto bpp = src.get_bytes_per_pixel();
        update_output_profile( f );

        if (auto tgt = prepare_target_frame(f, source, tgt_type))
        {
            switch( bpp )
            {
            case 1: {
                rotate_frame< 1 >( static_cast< uint8_t * >( const_cast< void * >( tgt.get_data() ) ),
                                   static_cast< const uint8_t * >( src.get_data() ),
                                   src.get_width(),
                                   src.get_height() );
                break;
            }

            case 2: {
                rotate_frame< 2 >( static_cast< uint8_t * >( const_cast< void * >( tgt.get_data() ) ),
                                   static_cast< const uint8_t * >( src.get_data() ) ,
                                   src.get_width(),
                                   src.get_height());
                break;
            }

            default:
                LOG_ERROR( "Rotation transform does not support format: "
                           + std::string( rs2_format_to_string( tgt.get_profile().format() ) ) );
            }
            return tgt;
        }
        return f;
    }

    void  rotation_filter::update_output_profile(const rs2::frame& f)
    {
        _source_stream_profile = f.get_profile();
        
        _target_stream_profile = _source_stream_profile.clone( _source_stream_profile.stream_type(),
                                                            _source_stream_profile.stream_index(),
                                                            _source_stream_profile.format() );


        auto src_vspi = dynamic_cast< video_stream_profile_interface * >( _source_stream_profile.get()->profile );
        if( ! src_vspi )
            throw std::runtime_error( "Stream profile interface is not video stream profile interface" );

        auto tgt_vspi = dynamic_cast< video_stream_profile_interface * >( _target_stream_profile.get()->profile );
        if( ! tgt_vspi )
            throw std::runtime_error( "Profile is not video stream profile" );

        rs2_intrinsics src_intrin = src_vspi->get_intrinsics();
        rs2_intrinsics tgt_intrin = tgt_vspi->get_intrinsics();

        // Adjust width and height based on the rotation angle 
        if( _value == 90 || _value == -90 )  // 90 or -90 degrees rotation
        {
            _rotated_width = src_intrin.height;
            _rotated_height = src_intrin.width;
            tgt_intrin.fx = src_intrin.fy;
            tgt_intrin.fy = src_intrin.fx;
            tgt_intrin.ppx = src_intrin.ppy;
            tgt_intrin.ppy = src_intrin.ppx;
        }
        else if( _value == 180 )  // 180 degrees rotation
        {
            _rotated_width = src_intrin.width;
            _rotated_height = src_intrin.height;
            tgt_intrin = src_intrin;
        }
        else { throw std::invalid_argument( "Unsupported rotation angle" ); }

        tgt_intrin.width = _rotated_width;
        tgt_intrin.height = _rotated_height;

        tgt_vspi->set_intrinsics( [tgt_intrin]() { return tgt_intrin; } );
        tgt_vspi->set_dims( _rotated_width, _rotated_height );
    }

    rs2::frame rotation_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source, rs2_extension tgt_type)
    {
        auto vf = f.as<rs2::video_frame>();
        auto ret = source.allocate_video_frame(_target_stream_profile, f,
            vf.get_bytes_per_pixel(),
            _rotated_width,
            _rotated_height,
            _rotated_width * vf.get_bytes_per_pixel(),
            tgt_type);

        return ret;
    }

    template< size_t SIZE >
    void rotation_filter::rotate_frame( uint8_t * const out, const uint8_t * source, int width, int height )
    {
        if( _value != 90 && _value != -90 && _value != 180 )
        {
            throw std::invalid_argument( "Invalid rotation angle. Only 90, -90, and 180 degrees are supported." );
        }

        // Define output dimensions
        int width_out = ( _value == 90 || _value == -90 ) ? height : width;  // rotate by 180 will keep the values as is
        int height_out = ( _value == 90 || _value == -90 ) ? width : height;  // rotate by 180 will keep the values as is

        // Perform rotation
        for( int i = 0; i < height; ++i ) 
        {
            for( int j = 0; j < width; ++j )
            {
                // Calculate source index
                size_t src_index = ( i * width + j ) * SIZE;

                // Determine output index based on rotation angle
                size_t out_index;
                if( _value == 90 )
                {
                    out_index = ( j * height + ( height - i - 1 ) ) * SIZE;
                }
                else if( _value == -90 )
                {
                    out_index = ( ( width - j - 1 ) * height + i ) * SIZE;
                }
                else  // 180 degrees
                {
                    out_index = ( ( height - i - 1 ) * width + ( width - j - 1 ) ) * SIZE;
                }

                // Copy pixel data from source to destination
                std::memcpy( &out[out_index], &source[src_index], SIZE );
            }
        }
    }
       
}

