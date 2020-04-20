// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

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

    private:
        void correct_motion(rs2::frame* f);

        std::shared_ptr<enable_motion_correction> _mm_correct_opt = nullptr;
        float3x3            _accel_sensitivity;
        float3              _accel_bias;
        float3x3            _gyro_sensitivity;
        float3              _gyro_bias;
        float3x3            _imu2depth_cs_alignment_matrix;     // Transform and align raw IMU axis [x,y,z] to be consistent with the Depth frame CS
    };

    class acceleration_transform : public motion_transform
    {
    public:
        acceleration_transform(std::shared_ptr<mm_calib_handler> mm_calib = nullptr, std::shared_ptr<enable_motion_correction> mm_correct_opt = nullptr);

    protected:
        acceleration_transform(const char* name, std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;

    };

    class gyroscope_transform : public motion_transform
    {
    public:
        gyroscope_transform(std::shared_ptr<mm_calib_handler> mm_calib = nullptr, std::shared_ptr<enable_motion_correction> mm_correct_opt = nullptr);

    protected:
        gyroscope_transform(const char* name, std::shared_ptr<mm_calib_handler> mm_calib, std::shared_ptr<enable_motion_correction> mm_correct_opt);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };
}
