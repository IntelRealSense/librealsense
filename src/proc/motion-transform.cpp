// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "ds5/ds5-options.h"
#include "ds5/ds5-motion.h"
#include "synthetic-stream.h"
#include "motion-transform.h"

namespace librealsense
{
    template<rs2_format FORMAT> void copy_hid_axes(byte * const dest[], const byte * source, double factor)
    {
        using namespace librealsense;
        auto hid = (hid_data*)(source);

        auto res = float3{ float(hid->x), float(hid->y), float(hid->z) } *float(factor);

        librealsense::copy(dest[0], &res, sizeof(float3));
    }

    // The Accelerometer input format: signed int 16bit. data units 1LSB=0.001g;
    // Librealsense output format: floating point 32bit. units m/s^2,
    template<rs2_format FORMAT> void unpack_accel_axes(byte * const dest[], const byte * source, int width, int height, int output_size)
    {
        static constexpr float gravity = 9.80665f;          // Standard Gravitation Acceleration
        static constexpr double accelerator_transform_factor = 0.001*gravity;

        copy_hid_axes<FORMAT>(dest, source, accelerator_transform_factor);
    }

    // The Gyro input format: signed int 16bit. data units 1LSB=0.1deg/sec;
    // Librealsense output format: floating point 32bit. units rad/sec,
    template<rs2_format FORMAT> void unpack_gyro_axes(byte * const dest[], const byte * source, int width, int height, int output_size)
    {
        static const double gyro_transform_factor = deg2rad(0.1);

        copy_hid_axes<FORMAT>(dest, source, gyro_transform_factor);
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
            // TODO Define L500 base transformation alignment
            _imu2depth_cs_alignment_matrix = { {1,0,0},{0,1,0}, {0,0,1} };
        }
    }

    rs2::frame motion_transform::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        auto&& ret = functional_processing_block::process_frame(source, f);
        correct_motion(&ret);

        return ret;
    }

    void motion_transform::correct_motion(rs2::frame* f)
    {
        auto xyz = (float3*)(f->get_data());

        // The IMU sensor orientation shall be aligned with depth sensor's coordinate system
        *xyz = _imu2depth_cs_alignment_matrix * (*xyz);

        // IMU calibration is done with data in depth sensor's coordinate system, so calibration parameters should be applied for motion correction
        // in the same coordinate system
        if (_mm_correct_opt)
        {
            if ((_mm_correct_opt->query() > 0.f)) // TBD resolve duality of is_enabled/is_active
            {
                auto&& s = f->get_profile().stream_type();
                if (s == RS2_STREAM_ACCEL)
                    *xyz = (_accel_sensitivity * (*xyz)) - _accel_bias;

                if (s == RS2_STREAM_GYRO)
                    *xyz = _gyro_sensitivity * (*xyz) - _gyro_bias;
            }
        }
    }

    acceleration_transform::acceleration_transform(std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : acceleration_transform("Acceleration Transform", mm_calib, mm_correct_opt)
    {}

    acceleration_transform::acceleration_transform(const char * name, std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : motion_transform(name, RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, mm_calib, mm_correct_opt)
    {}

    void acceleration_transform::process_function(byte * const dest[], const byte * source, int width, int height, int output_size, int actual_size)
    {
        unpack_accel_axes<RS2_FORMAT_MOTION_XYZ32F>(dest, source, width, height, actual_size);
    }

    gyroscope_transform::gyroscope_transform(std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : gyroscope_transform("Gyroscope Transform", mm_calib, mm_correct_opt)
    {}

    gyroscope_transform::gyroscope_transform(const char * name, std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt)
        : motion_transform(name, RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, mm_calib, mm_correct_opt)
    {}

    void gyroscope_transform::process_function(byte * const dest[], const byte * source, int width, int height, int output_size, int actual_size)
    {
        unpack_gyro_axes<RS2_FORMAT_MOTION_XYZ32F>(dest, source, width, height, actual_size);
    }
}

