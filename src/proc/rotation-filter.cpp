// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <librealsense2/hpp/rs_sensor.hpp>
#include "option.h"
#include "stream.h"
#include "core/video.h"
#include "proc/rotation-filter.h"
#include <rsutils/easylogging/easyloggingpp.h>

namespace librealsense {

    const int rotation_min_val = -90;
    const int rotation_max_val = 180;  
    const int rotation_default_val = 0;
    const int rotation_step = 90; 


    rotation_filter::rotation_filter()
        : rotation_filter( std::vector< rs2_stream >{ RS2_STREAM_DEPTH } )
    {
    }

    rotation_filter::rotation_filter( std::vector< rs2_stream > streams_to_rotate )
        : stream_filter_processing_block( "Rotation Filter" )
        , _streams_to_rotate( streams_to_rotate )
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

                if( ! strong_rotation_control->is_valid( val ) )
                    throw invalid_value_exception( rsutils::string::from()
                                                   << "Unsupported rotation scale " << val << " is out of range." );

                _value = val;
            } );

        register_option( RS2_OPTION_ROTATION, rotation_control );
    }

    rs2::frame rotation_filter::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        if( _value == rotation_default_val || _streams_to_rotate.empty() )
            return f;

        // Copying to a local variable to avoid locking.
        float local_value = _value;

        auto src = f.as< rs2::video_frame >();
        rs2::stream_profile profile = f.get_profile();
        
        auto bpp = src.get_bytes_per_pixel();
        update_output_profile( f, local_value );
        rs2_stream type = profile.stream_type();
        rs2_extension tgt_type;
        if( type == RS2_STREAM_COLOR || type == RS2_STREAM_INFRARED )
            tgt_type = RS2_EXTENSION_VIDEO_FRAME;
        else
            tgt_type = f.is< rs2::disparity_frame >() ? RS2_EXTENSION_DISPARITY_FRAME : RS2_EXTENSION_DEPTH_FRAME;

        if (auto tgt = prepare_target_frame(f, source, tgt_type))
        {
            auto format = profile.format();
            if( format == RS2_FORMAT_YUYV && ( local_value == 90 || local_value == -90 ) )
            {
                LOG_ERROR( "Rotating YUYV format is disabled for 90 or -90 degrees" );
                return f;
            }

            if( format == RS2_FORMAT_YUYV )
            {
                rotate_YUYV_frame( static_cast< uint8_t * >( const_cast< void * >( tgt.get_data() ) ),
                                   static_cast< const uint8_t * >( src.get_data() ),
                                   src.get_width(),
                                   src.get_height() );
            }
            else
            {
                rotate_frame( static_cast< uint8_t * >( const_cast< void * >( tgt.get_data() ) ),
                              static_cast< const uint8_t * >( src.get_data() ),
                              src.get_width(),
                              src.get_height(),
                              bpp,
                              local_value );
            }
            return tgt;
        }
        return f;
    }

    void  rotation_filter::update_output_profile(const rs2::frame& f, float & value)
    {
        _source_stream_profile = f.get_profile();
        auto stream_type = _source_stream_profile.stream_type();
        auto stream_index = _source_stream_profile.stream_index();
        std::pair< rs2_stream, int > stream_key( stream_type, stream_index );

        // If the map is empty last rotation value is 0
        float last_rotation = 0.0f;
        auto it = _last_rotation_values.find( stream_key );
        if( it != _last_rotation_values.end() )
            last_rotation = it->second;

        // If the current rotation value is already applied, do nothing
        if( last_rotation == value )
            return;
        
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

        if( value == 90 )
        {
            _rotated_width = src_intrin.height;
            _rotated_height = src_intrin.width;

            tgt_intrin.fx = src_intrin.fy;
            tgt_intrin.fy = src_intrin.fx;

            tgt_intrin.ppx = src_intrin.height - src_intrin.ppy;
            tgt_intrin.ppy = src_intrin.ppx;
        }
        else if( value == -90 )
        {
            _rotated_width = src_intrin.height;
            _rotated_height = src_intrin.width;

            tgt_intrin.fx = src_intrin.fy;
            tgt_intrin.fy = src_intrin.fx;

            tgt_intrin.ppx = src_intrin.ppy;
            tgt_intrin.ppy = src_intrin.width - src_intrin.ppx;
        }
        else if( value == 180 )  // 180 degrees rotation
        {
            _rotated_width = src_intrin.width;
            _rotated_height = src_intrin.height;

            tgt_intrin.fx = src_intrin.fx;
            tgt_intrin.fy = src_intrin.fy;
            tgt_intrin.ppx = src_intrin.width - src_intrin.ppx;
            tgt_intrin.ppy = src_intrin.height - src_intrin.ppy;
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

    void rotation_filter::rotate_frame( uint8_t * const out, const uint8_t * source, int width, int height, int bpp, float & value )
    {
        if( value != 90 && value != -90 && value != 180 )
        {
            throw std::invalid_argument( "Invalid rotation angle. Only 90, -90, and 180 degrees are supported." );
        }

        // Define output dimensions
        int width_out = ( value == 90 || value == -90 ) ? height : width;    // rotate by 180 will keep the values as is
        int height_out = ( value == 90 || value == -90 ) ? width : height;  // rotate by 180 will keep the values as is

        // Perform rotation
        for( int i = 0; i < height; ++i )
        {
            for( int j = 0; j < width; ++j )
            {
                // Calculate source index
                size_t src_index = ( i * width + j ) * bpp;

                // Determine output index based on rotation angle
                size_t out_index;
                if( value == 90 )
                {
                    out_index = ( j * height + ( height - i - 1 ) ) * bpp;
                }
                else if( value == -90 )
                {
                    out_index = ( ( width - j - 1 ) * height + i ) * bpp;
                }
                else  // 180 degrees
                {
                    out_index = ( ( height - i - 1 ) * width + ( width - j - 1 ) ) * bpp;
                }

                // Copy pixel data from source to destination
                std::memcpy( &out[out_index], &source[src_index], bpp );
            }
        }
    }


    void rotation_filter::rotate_YUYV_frame( uint8_t * const out, const uint8_t * source, int width, int height ) 
    {
        int frame_size = width * height * 2;  // YUYV has 2 bytes per pixel
        for( int i = 0; i < frame_size; i += 4 )
        {
            // Find the corresponding flipped position in the output
            int flipped_index = frame_size - 4 - i;

            // Copy YUYV pixels in reverse order to rotate 180 degrees
            out[flipped_index] = source[i];          // Y1
            out[flipped_index + 1] = source[i + 1];  // U
            out[flipped_index + 2] = source[i + 2];  // Y2
            out[flipped_index + 3] = source[i + 3];  // V
        }
    }


    bool rotation_filter::should_process( const rs2::frame & frame )
    {
        if( ! frame || frame.is< rs2::frameset >() )
            return false;
        auto type = frame.get_profile().stream_type();

        for( auto & stream_type : _streams_to_rotate ){
            if( stream_type == type )
                return true;
        }
        return false;
    }
       
}

