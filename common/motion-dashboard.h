// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


namespace rs2
{
    class stream_dashboard;

    class motion_dashboard : public stream_dashboard
    {
    public:

        motion_dashboard( std::string name, enum rs2_stream stream);

        // Draw Accelerate dashboard and graph's lines
        void draw( ux_window & window, rect rectangle ) override;
        int get_height() const override;
        void clear( bool full ) override;

        // Show radio buttons for X, Y, Z and N lines
        void show_radiobuttons();
        // Show slider that change pause between acceleration values that we saving.
        void show_data_rate_slider();

        // Extract X, Y, Z and calculate N values from a frame
        void process_frame( rs2::frame f ) override;

    protected:
        float x_value;
        float y_value;
        float z_value;
        float n_value;  // Norm ||V|| = SQRT(X^2 + Y^2 + Z^2)

        float dashboard_update_rate;
        double last_time;

        const char * x_axes_name = "X";
        const char * y_axes_name = "Y";
        const char * z_axes_name = "Z";
        const char * n_axes_name = "N";

        std::vector< std::string > plots = { x_axes_name, y_axes_name, z_axes_name, n_axes_name };
        int plot_index = 0;

        enum rs2_stream stream_type;
        const float MIN_FRAME_RATE = 0.01f;
        const float MAX_FRAME_RATE = 0.1f;

        const int DEQUE_SIZE = 200;
        std::deque< float > x_history;
        std::deque< float > y_history;
        std::deque< float > z_history;
        std::deque< float > n_history;
    };
}