// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/rs.hpp>
#include "notifications.h"
#include <rsutils/number/float3.h>

namespace librealsense { namespace ds { struct mini_intrinsics; } }
namespace librealsense { namespace ds { enum class d500_calibration_distortion; } }

namespace rs2
{
    class ux_window;

    class calibration_model
    {
        using float3x3 = rsutils::number::float3x3;
        using mini_intrinsics = librealsense::ds::mini_intrinsics;

    public:
        calibration_model(rs2::device dev, std::shared_ptr<notifications_model> not_model);

        bool supports();

        void update(ux_window& window, std::string& error_message);

        void open() { to_open = true; }

    private:
        void draw_float4x4(std::string name, float3x3 & field, const float3x3& original, bool& changed);
        void draw_intrinsics( std::string name, mini_intrinsics & field, const mini_intrinsics & original, bool & changed );
        void draw_distortion(std::string name, librealsense::ds::d500_calibration_distortion& distortion_model, float(&distortion_coeffs)[13], const float(&original_coeffs)[13],bool& changed);
        void draw_float(std::string name, float& x, const float& orig, bool& changed);
        void draw_read_only_float(std::string name, float x);
        void draw_int( std::string name, uint16_t & x, const uint16_t & orig, bool & changed );
        void draw_read_only_int(std::string name, int x);
        void d400_update(ux_window& window, std::string& error_message);
        void d500_update(ux_window& window, std::string& error_message);

        rs2::device dev;
        bool to_open = false;
        bool _accept = false;

        bool d400_device = false;
        bool d500_device = false;

        std::vector<uint8_t> _calibration;
        std::vector<uint8_t> _original;

        int selected_resolution = 0;

        std::weak_ptr<notifications_model> _not_model;
    };
}
