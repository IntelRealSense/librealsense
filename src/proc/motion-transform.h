// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

#include "ds5/ds5-motion.h"

namespace librealsense
{
    class motion_transform : public functional_processing_block
    {
    public:
        motion_transform(rs2_format target_format, rs2_stream target_stream, std::shared_ptr<mm_calib_handler> mm_calib = nullptr, bool is_motion_correction_enabled = false);

    protected:
        motion_transform(const char* name, rs2_format target_format, rs2_stream target_stream, std::shared_ptr<mm_calib_handler> mm_calib, bool is_motion_correction_enabled);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        void correct_motion(rs2::frame* f);

        std::shared_ptr<mm_calib_handler> _mm_calib = nullptr;
        bool _is_motion_correction_enabled = false;
    };

    class acceleration_transform : public motion_transform
    {
    public:
        acceleration_transform(std::shared_ptr<mm_calib_handler> mm_calib = nullptr, bool is_motion_correction_enabled = false);

    protected:
        acceleration_transform(const char* name, std::shared_ptr<mm_calib_handler> mm_calib, bool is_motion_correction_enabled);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size) override;
    };

    class gyroscope_transform : public motion_transform
    {
    public:
        gyroscope_transform(std::shared_ptr<mm_calib_handler> mm_calib = nullptr, bool is_motion_correction_enabled = false);

    protected:
        gyroscope_transform(const char* name, std::shared_ptr<mm_calib_handler> mm_calib, bool is_motion_correction_enabled);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size) override;
    };
}