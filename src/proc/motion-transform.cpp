// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "ds/d400/d400-options.h"
#include "ds/d400/d400-motion.h"
#include "synthetic-stream.h"
#include "motion-transform.h"
#include "stream.h"
#include <src/platform/hid-data.h>
#include <src/core/frame-processor-callback.h>


namespace librealsense
{
    template<rs2_format FORMAT> void copy_hid_axes( uint8_t * const dest[], const uint8_t * source, double factor, bool high_accuracy, bool is_mipi)
    {
        using namespace librealsense;

        auto res = float3();

        if (is_mipi)
        {
           if( ! high_accuracy )
            {
                auto hid = (hid_mipi_data *)( source );
                res = float3{ float( hid->x ), float( hid->y ), float( hid->z ) } * float( factor );
            }
            else
            {
                auto hid = (hid_mipi_data_32 *)( source );
                res = float3{ float( hid->x ), float( hid->y ), float( hid->z ) } * float( factor );
            }
        }
        else
        {
            auto hid = (hid_data*)(source);
            //since D400 FW version 5.16 the hid report struct changed to 32 bit for each paramater.
            //To support older FW versions we convert the data to int16_t before casting to float as we only get valid data at the lower 16  bits.
            if( ! high_accuracy )
            {
                hid->x = static_cast< int16_t >( hid->x );
                hid->y = static_cast< int16_t >( hid->y );
                hid->z = static_cast< int16_t >( hid->z );
            }

            res = float3{ float( hid->x ), float( hid->y ), float( hid->z ) } * float( factor );
        }

        std::memcpy( dest[0], &res, sizeof( float3 ) );
    }

    // The Accelerometer input format: signed int 16bit. data units 1LSB=0.001g;
    // Librealsense output format: floating point 32bit. units m/s^2,
    template<rs2_format FORMAT> void unpack_accel_axes( uint8_t * const dest[], const uint8_t * source, int width, int height, int output_size, bool high_accuracy, bool is_mipi = false)
    {
        static constexpr float gravity = 9.80665f;          // Standard Gravitation Acceleration
        static constexpr double accelerator_transform_factor = 0.001*gravity;

        copy_hid_axes< FORMAT >( dest, source, accelerator_transform_factor, high_accuracy, is_mipi );
    }

    // The Gyro input format: signed int 16bit. data units depends on set sensitivity;
    // Librealsense output format: floating point 32bit. units rad/sec,
    template< rs2_format FORMAT >
    void unpack_gyro_axes( uint8_t * const dest[], const uint8_t * source, int width, int height, int output_size,
                           double gyro_scale_factor = 0.1, bool is_mipi = false )
    {
        const double gyro_transform_factor = deg2rad( gyro_scale_factor );
        //high_accuracy=true when gyro_scale_factor=0.001 for FW version >=5.16 
        //high_accuracy=false when gyro_scale_factor=0.01 for FW version <5.16
        double high_accuracy = ( gyro_scale_factor != 0.1 );
        copy_hid_axes< FORMAT >( dest, source, gyro_transform_factor, high_accuracy, is_mipi );
    }

    motion_transform::motion_transform(rs2_format target_format, rs2_stream target_stream,
        std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : motion_transform("Motion Transform", target_format, target_stream, mm_calib, mm_correct_opt)
    {}

    motion_transform::motion_transform(const char* name, rs2_format target_format, rs2_stream target_stream,
        std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : functional_processing_block(name, target_format, target_stream, RS2_EXTENSION_MOTION_FRAME),
        _mm_correct_opt(mm_correct_opt)
    {
        if (mm_calib)
        {
            _imu2depth_cs_alignment_matrix = (*mm_calib).imu_to_depth_alignment();
            if (_mm_correct_opt)
            {
                auto accel_intr = (*mm_calib).get_intrinsic(RS2_STREAM_ACCEL);
                auto gyro_intr = (*mm_calib).get_intrinsic(RS2_STREAM_GYRO);
                _accel_sensitivity  = accel_intr.sensitivity;
                _accel_bias         = accel_intr.bias;
                _gyro_sensitivity   = gyro_intr.sensitivity;
                _gyro_bias          = gyro_intr.bias;
            }
        }
        else
        {
            _imu2depth_cs_alignment_matrix = { {1,0,0},{0,1,0}, {0,0,1} };
        }
    }

    rs2::frame motion_transform::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        auto&& ret = functional_processing_block::process_frame(source, f);
        correct_motion(&ret);

        return ret;
    }

    void motion_transform::correct_motion_helper(float3* xyz, rs2_stream stream_type) const
    {
        // The IMU sensor orientation shall be aligned with depth sensor's coordinate system
        *xyz = _imu2depth_cs_alignment_matrix * (*xyz);

        // IMU calibration is done with data in depth sensor's coordinate system, so calibration parameters should be applied for motion correction
        // in the same coordinate system
        if (_mm_correct_opt)
        {
            if ((_mm_correct_opt->query() > 0.f)) // TBD resolve duality of is_enabled/is_active
            {
                if (stream_type == RS2_STREAM_ACCEL)
                    *xyz = (_accel_sensitivity * (*xyz)) - _accel_bias;

                if (stream_type == RS2_STREAM_GYRO)
                    *xyz = _gyro_sensitivity * (*xyz) - _gyro_bias;
            }
        }
    }
    void motion_transform::correct_motion(rs2::frame* f) const
    {
        auto xyz = (float3*)(f->get_data());

        correct_motion_helper(xyz, f->get_profile().stream_type());
    }

    void motion_to_accel_gyro::correct_motion(float3* xyz) const
    {
        correct_motion_helper(xyz, _accel_gyro_target_profile->get_stream_type());
    }

    motion_to_accel_gyro::motion_to_accel_gyro( std::shared_ptr< mm_calib_handler > mm_calib,
                                                std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                                double gyro_scale_factor )
        : motion_to_accel_gyro( "Accel_Gyro Transform", mm_calib, mm_correct_opt, gyro_scale_factor )
    {}

    motion_to_accel_gyro::motion_to_accel_gyro( const char * name,
                                                std::shared_ptr< mm_calib_handler > mm_calib,
                                                std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                                double gyro_scale_factor )
        : motion_transform(name, RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ANY, mm_calib, mm_correct_opt)
        , _gyro_scale_factor(gyro_scale_factor)
    {
        configure_processing_callback();
    }

    void motion_to_accel_gyro::configure_processing_callback()
    {
        // define and set the frame processing callback
        auto process_callback = [&](frame_holder && frame, synthetic_source_interface* source)
        {
            auto profile = As<motion_stream_profile, stream_profile_interface>(frame.frame->get_stream());
            if (!profile)
            {
                LOG_ERROR("Failed configuring accel_gyro processing block: ", get_info(RS2_CAMERA_INFO_NAME));
                return;
            }

            if (profile.get() != _source_stream_profile.get())
            {
                _source_stream_profile = profile;
                // creating profile that fits the frame real stream type
                _accel_gyro_target_profile = profile->clone();
                _accel_gyro_target_profile->set_format(_target_format);
                _accel_gyro_target_profile->set_stream_index(0);
                //_accel_gyro_target_profile->set_unique_id(_target_profile_idx);
            }

            if (frame->get_frame_data()[0] == 1)
            {
                _accel_gyro_target_profile->set_stream_type(RS2_STREAM_ACCEL);
            }
            else if (frame->get_frame_data()[0] == 2)
            {
                _accel_gyro_target_profile->set_stream_type(RS2_STREAM_GYRO);
            }
            else
            {
                throw("motion_to_accel_gyro::configure_processing_callback - stream type not discovered");
            }

            frame_holder agf;
            agf = source->allocate_motion_frame(_accel_gyro_target_profile, frame);

            // process the frame
            uint8_t * frame_data[1];
            frame_data[0] = (uint8_t *)agf.frame->get_frame_data();
            process_function(frame_data, (const uint8_t *)frame->get_frame_data(), 0, 0, 0, 0);

            // correct the axes values according to the device's data
            correct_motion((float3*)(frame_data[0]));

            source->frame_ready(std::move(agf));
        };

        set_processing_callback( make_frame_processor_callback( std::move( process_callback ) ) );
    }

    void motion_to_accel_gyro::process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int output_size, int actual_size)
    {
        if (source[0] == 1)
        {
            _target_stream = RS2_STREAM_ACCEL;
            bool high_accuracy = ( _gyro_scale_factor != 0.1 );
            unpack_accel_axes< RS2_FORMAT_MOTION_XYZ32F >( dest, source, width, height, actual_size, high_accuracy, true );
        }
        else if (source[0] == 2)
        {
            _target_stream = RS2_STREAM_GYRO;
            unpack_gyro_axes<RS2_FORMAT_MOTION_XYZ32F>( dest, source, width, height, actual_size,
                                                       _gyro_scale_factor, true );
        }
        else
        {
            throw("motion_to_accel_gyro::process_function - stream type not discovered");
        }
    }

    acceleration_transform::acceleration_transform( std::shared_ptr< mm_calib_handler > mm_calib,
                                                    std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                                    bool high_accuracy )
        : acceleration_transform( "Acceleration Transform", mm_calib, mm_correct_opt, high_accuracy )
    {}

    acceleration_transform::acceleration_transform(const char * name, std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt, bool high_accuracy)
        : motion_transform( name, RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, mm_calib, mm_correct_opt)
        , _high_accuracy( high_accuracy )
    {}

    void acceleration_transform::process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int output_size, int actual_size )
    {
        unpack_accel_axes< RS2_FORMAT_MOTION_XYZ32F >( dest, source, width, height, actual_size, _high_accuracy );
    }

    gyroscope_transform::gyroscope_transform( std::shared_ptr< mm_calib_handler > mm_calib,
                                              std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                              double gyro_scale_factor )
        : gyroscope_transform( "Gyroscope Transform", mm_calib, mm_correct_opt, gyro_scale_factor )
    {}

    gyroscope_transform::gyroscope_transform( const char * name,
                                              std::shared_ptr< mm_calib_handler > mm_calib,
                                              std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                              double gyro_scale_factor )
        : motion_transform(name, RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, mm_calib, mm_correct_opt)
        , _gyro_scale_factor( gyro_scale_factor )
    {}

    void gyroscope_transform::process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int output_size, int actual_size )
    {
        unpack_gyro_axes< RS2_FORMAT_MOTION_XYZ32F >( dest,
                                                      source,
                                                      width,
                                                      height,
                                                      actual_size,
                                                      _gyro_scale_factor );
    }
}

