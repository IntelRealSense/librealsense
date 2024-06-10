// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/rs.hpp>
#include "notifications.h"
#include <rsutils/number/float3.h>


namespace rs2
{
    class ux_window;

    class calibration_model
    {
        using float3x3 = rsutils::number::float3x3;

    public:
        calibration_model(rs2::device dev, std::shared_ptr<notifications_model> not_model);

        bool supports();

        void update(ux_window& window, std::string& error_message);

        void open() { to_open = true; }

    private:
        void draw_float4x4(std::string name, float3x3 & feild, const float3x3& original, bool& changed);
        void draw_float(std::string name, float& x, const float& orig, bool& changed);

        rs2::device dev;
        bool to_open = false;
        bool _accept = false;

        std::vector<uint8_t> _calibration;
        std::vector<uint8_t> _original;

        int selected_resolution = 0;

        std::weak_ptr<notifications_model> _not_model;
    };
}
