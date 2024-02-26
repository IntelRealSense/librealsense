// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "output-model.h"
#include "device-model.h"

namespace rs2
{
    class accel_dashboard : public stream_dashboard
    {
    public:

        accel_dashboard( std::string name );

        // Extract X, Y, Z and calculate N values from a frame
        void process_frame( rs2::frame f ) override;

        // Draw Accelerate dashboard and choose graph's lines
        void draw( ux_window & window, rect rectangle ) override;
        int get_height() const override;
        void clear( bool full ) override;

        // Show radio buttons for X, Y, Z and N lines
        void show_radiobuttons();
        // Show slider that change pause between acceleration values that we saving.
        void show_data_rate_slider();

    private:
        float x_value;
        float y_value;
        float z_value;
        float n_value;  // Norm ||V|| = SQRT(X^2 + Y^2 + Z^2)

        float frame_rate;
        double last_time;
        std::map< float, double > frame_to_time;

        std::vector< std::string > accel_params = { "X", "Y", "Z", "N" };
        int curr_accel_param_position = 0;

        const float MIN_FRAME_RATE = 0.01f;
        const float MAX_FRAME_RATE = 0.1f;

        const int DEQUE_SIZE = 200;
        std::deque< float > x_history;
        std::deque< float > y_history;
        std::deque< float > z_history;
        std::deque< float > n_history;
    };
}