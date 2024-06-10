// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once
#include "synthetic-stream.h"

namespace librealsense
{
    class enable_motion_correction;
    class mm_calib_handler;
    class functional_processing_block;

    class motion_transform : public functional_processing_block
    {
    public:
        motion_transform(rs2_format target_format, rs2_stream target_stream,
            std::shared_ptr<mm_calib_handler> mm_calib = nullptr,
            std::shared_ptr<enable_motion_correction> mm_correct_opt = nullptr);

    protected:
        motion_transform(const char* name, rs2_format target_format, rs2_stream target_stream,
            std::shared_ptr<mm_calib_handler> mm_calib,
            std::shared_ptr<enable_motion_correction> mm_correct_opt);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    protected:
        void correct_motion(rs2::frame* f) const;
        void correct_motion_helper(float3* xyz, rs2_stream stream_type) const;

        std::shared_ptr<enable_motion_correction> _mm_correct_opt = nullptr;
        float3x3            _accel_sensitivity;
        float3              _accel_bias;
        float3x3            _gyro_sensitivity;
        float3              _gyro_bias;
        float3x3            _imu2depth_cs_alignment_matrix;     // Transform and align raw IMU axis [x,y,z] to be consistent with the Depth frame CS
    };

    class motion_to_accel_gyro : public motion_transform
    {
    public:
        motion_to_accel_gyro( std::shared_ptr< mm_calib_handler > mm_calib = nullptr,
                              std::shared_ptr< enable_motion_correction > mm_correct_opt = nullptr,
                              double gyro_scale_factor = 0.1 );

    protected:
        motion_to_accel_gyro( const char * name,
                              std::shared_ptr< mm_calib_handler > mm_calib,
                              std::shared_ptr< enable_motion_correction > mm_correct_opt,
                              double gyro_scale_factor );
        void configure_processing_callback();
        void process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size ) override;
        void correct_motion(float3* xyz) const;

        std::shared_ptr<stream_profile_interface> _source_stream_profile;
        std::shared_ptr<stream_profile_interface> _accel_gyro_target_profile;
        double _gyro_scale_factor = 0.1;
    };

    class acceleration_transform : public motion_transform
    {
    public:
        acceleration_transform( std::shared_ptr< mm_calib_handler > mm_calib = nullptr,
                                std::shared_ptr< enable_motion_correction > mm_correct_opt = nullptr,
                                bool high_accuracy = false );

    protected:
        acceleration_transform( const char * name,
                                std::shared_ptr< mm_calib_handler > mm_calib,
                                std::shared_ptr< enable_motion_correction > mm_correct_opt,
                                bool high_accuracy );
        void process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size) override;
        //To be refactored and change to accel_scale_factor once we implement sensitivity feature for the accel like the gyro
        bool _high_accuracy = false;

    };

    class gyroscope_transform : public motion_transform
    {
    public:
        gyroscope_transform( std::shared_ptr< mm_calib_handler > mm_calib = nullptr,
                             std::shared_ptr< enable_motion_correction > mm_correct_opt = nullptr,
                             double gyro_scale_factor = 0.1 );

    protected:
        gyroscope_transform( const char * name,
                             std::shared_ptr< mm_calib_handler > mm_calib,
                             std::shared_ptr< enable_motion_correction > mm_correct_opt,
                             double gyro_scale_factor );
                             
        void process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size ) override;

        double  _gyro_scale_factor = 0.1;
    };
}
