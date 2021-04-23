// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "notifications.h"
#include "../src/concurrency.h"
#include "../src/algo.h"

#include <random>

namespace rs2
{
    class viewer_model;
    class subdevice_model;
    struct subdevice_ui_selection;

    // On-chip Calibration manager owns the background thread
    // leading the calibration process
    // It is controlled by autocalib_notification_model UI object
    // that invokes the process when needed
    class on_chip_calib_manager : public process_manager
    {
    public:
        on_chip_calib_manager(viewer_model& viewer, std::shared_ptr<subdevice_model> sub, device_model& model, device dev, std::shared_ptr<subdevice_model> sub_color = nullptr);
        ~on_chip_calib_manager();

        bool allow_calib_keep() const { return true; }

        // Get health number from the calibration summary
        float get_health() const { return _health; }
        float get_health_1() const { return _health_1; }
        float get_health_2() const { return _health_2; }
        float get_health_uvmapping(int idx) const { return _health_uvmapping[idx]; }

        // Write new calibration to the device
        void keep();
        void keep_uvmapping_calib();

        // Restore Viewer UI to how it was before auto-calib
        void restore_workspace(invoker invoke);
        
        // Ask the firmware to use one of the before/after calibration tables
        void apply_calib(bool use_new);

        // Get depth metrics for before/after calibration tables
        std::pair<float, float> get_metric(bool use_new);

        void update_last_used();

        float ground_truth = 1200.0f;
        int average_step_count = 20;
        int step_count = 20;
        int accuracy = 2;
        int speed = 3;
        int speed_fl = 1;
        bool intrinsic_scan = true;
        bool apply_preset = true;

        enum auto_calib_action
        {
            RS2_CALIB_ACTION_ON_CHIP_OB_CALIB,  // On-Chip calibration extended
            RS2_CALIB_ACTION_ON_CHIP_CALIB,     // On-Chip calibration
            RS2_CALIB_ACTION_ON_CHIP_FL_CALIB,  // On-Chip focal length calibration
            RS2_CALIB_ACTION_TARE_CALIB,        // Tare calibration
            RS2_CALIB_ACTION_TARE_GROUND_TRUTH, // Tare ground truth
            RS2_CALIB_ACTION_FL_CALIB,          // Focal length calibration
            RS2_CALIB_ACTION_UVMAPPING,         // UVMapping calibration
        };

        auto_calib_action action = RS2_CALIB_ACTION_ON_CHIP_CALIB;
        float laser_status_prev = 0.0f;

        int fl_step_count = 51;
        int fy_scan_range = 40;
        int keep_new_value_after_sucessful_scan = 1;
        int fl_data_sampling = 1;
        int adjust_both_sides = 0;

        int fl_scan_location = 0;
        int fy_scan_direction = 0;
        int white_wall_mode = 0;
        
        int retry_times = 0;
        bool toggle = false;

        float ratio = 0.0f;
        float align = 0.0f;

        const float correction_factor = 0.50f;
        float corrected_ratio = 0.0f;
        float tilt_angle = 0.0f;

        std::shared_ptr<subdevice_model> _sub;
        std::shared_ptr<subdevice_model> _sub_color;

        void calibrate();
        void calibrate_fl();
        void calibrate_uvmapping();
        void get_ground_truth();

        void turn_roi_on();
        void turn_roi_off();

    private:

        std::vector<uint8_t> safe_send_command(const std::vector<uint8_t>& cmd, const std::string& name);

        rs2::depth_frame fetch_depth_frame(invoker invoke);

        std::pair<float, float> get_depth_metrics(invoker invoke);

        void process_flow(std::function<void()> cleanup, invoker invoke) override;

        float _health = -1.0f;
        float _health_1 = -1.0f;
        float _health_2 = -1.0f;

        float _health_uvmapping[4] = { -0.1f, -0.1f, -0.1f, -0.1f };
        std::vector<uint8_t> color_intrin_raw_data;

        device _dev;

        bool _was_streaming = false;
        bool _synchronized = false;
        bool _post_processing = false;
        std::shared_ptr<subdevice_ui_selection> _ui { nullptr };
        bool _in_3d_view = false;
        int _uid = 0;
        int _uid2 = 0;
        int _uid_color = 0;
        std::shared_ptr<subdevice_ui_selection> _ui_color{ nullptr };

        viewer_model& _viewer;

        std::vector<uint8_t> _old_calib, _new_calib;
        std::vector<std::pair<float, float>> _metrics;
        device_model& _model;

        bool _restored = true;

        float _ppx = 0.0f;
        float _ppy = 0.0f;
        float _fx = 0.0f;
        float _fy = 0.0f;

        void stop_viewer(invoker invoke);
        bool start_viewer(int w, int h, int fps, invoker invoke);
        void try_start_viewer(int w, int h, int fps, invoker invoke);

        void undistort(uint8_t* img, int width, int height, const rs2_intrinsics& intrin, int roi_ws, int roi_hs, int roi_we, int roi_he);
        void find_z_at_corners(float left_x[4], float left_y[4], int width, int num, std::vector<std::vector<uint16_t>> & depth, float left_z[4]);
        void get_and_update_color_intrinsics(uint32_t width, uint32_t height, float ppx, float ppy, float fx, float fy);
    };

    // Auto-calib notification model is managing the UI state-machine
    // controling auto-calibration
    struct autocalib_notification_model : public process_notification_model
    {
        enum auto_calib_ui_state
        {
            RS2_CALIB_STATE_INITIAL_PROMPT,  // First screen, would you like to run Health-Check?
            RS2_CALIB_STATE_FAILED,          // Failed, show _error_message
            RS2_CALIB_STATE_COMPLETE,        // After write, quick blue notification
            RS2_CALIB_STATE_CALIB_IN_PROCESS,// Calibration in process... Shows progressbar
            RS2_CALIB_STATE_CALIB_COMPLETE,  // Calibration complete, show before/after toggle and metrics
            RS2_CALIB_STATE_TARE_INPUT,      // Collect input parameters for Tare calib
            RS2_CALIB_STATE_TARE_INPUT_ADVANCED,      // Collect input parameters for Tare calib
            RS2_CALIB_STATE_SELF_INPUT,      // Collect input parameters for Self calib
            RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH,            // Calculating ground truth
            RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS, // Calculating ground truth in process... Shows progressbar
            RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_COMPLETE,   // Calculating ground truth complete, show succeeded or failed
            RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED,     // Failed to calculating the ground truth
            RS2_CALIB_STATE_FL_INPUT,        // Collect input parameters for focal length calib
            RS2_CALIB_STATE_UVMAPPING_INPUT, // Collect input parameters for UVMapping calibration with specific target
        };

        autocalib_notification_model(std::string name,
            std::shared_ptr<on_chip_calib_manager> manager, bool expaned);

        on_chip_calib_manager& get_manager() { 
            return *std::dynamic_pointer_cast<on_chip_calib_manager>(update_manager); 
        }

        void set_color_scheme(float t) const override;
        void draw_content(ux_window& win, int x, int y, float t, std::string& error_message) override;
        void draw_dismiss(ux_window& win, int x, int y) override;
        void draw_expanded(ux_window& win, std::string& error_message) override;
        void draw_intrinsic_extrinsic(int x, int y);
        int calc_height() override;
        void dismiss(bool snooze) override;

        bool use_new_calib = true;
        std::string _error_message = "";
    };

    // Class for calculating the rectangle sides on the specific target
    class rect_calculator
    {
    public:
        rect_calculator() {}
        virtual ~rect_calculator() {}

        int calculate(const rs2_frame * frame_ref, float rect_sides[4]); // return 0 if the target is not in the center, 1 if found, 2 if found and the rectangle sides are updated

    public:
        static const int _frame_num = 25;

    private:
        void calculate_rect_sides(float rect_sides[4]);

        int _width = 0;
        int _height = 0;

        float _rec_sides[_frame_num][4];
        int _rec_idx = 0;
        int _rec_num = 0;
        const int _reset_limit = 10;
    };

    // Class for calculating the four Gaussian dot center locations on the specific target
    class dots_calculator
    {
    public:
        dots_calculator() {}
        virtual ~dots_calculator() {}

        int calculate(const rs2_frame* frame_ref, float dots_x[4], float dots_y[4]); // return 0 if the target is not in the center, 1 if found, 2 if dots positions are updated

    public:
        static const int _frame_num = 12;

    private:
        void calculate_dots_position(float dots_x[4], float dots_y[4]);

        int _width = 0;
        int _height = 0;

        float _dots_x[_frame_num][4];
        float _dots_y[_frame_num][4];
        int _rec_idx = 0;
        int _rec_num = 0;
        const int _reset_limit = 10;
    };

    class uvmapping_calib
    {
    public:
        uvmapping_calib(int pt_num, const float* left_x, const float* left_y, const float* left_z, const float* color_x, const float* color_y, const rs2_intrinsics& left_intrin, const rs2_intrinsics& color_intrin, rs2_extrinsics& extrin);
        virtual ~uvmapping_calib() {}

        bool calibrate(float & err_before, float & err_after, float& ppx, float& ppy, float& fx, float& fy);

    private:
        const float _max_change = 16.0f;

        int _pt_num;

        std::vector<float> _left_x;
        std::vector<float> _left_y;
        std::vector<float> _left_z;
        std::vector<float> _color_x;
        std::vector<float> _color_y;        
        
        rs2_intrinsics _left_intrin;
        rs2_intrinsics _color_intrin;
        rs2_extrinsics _extrin;
    };
}
