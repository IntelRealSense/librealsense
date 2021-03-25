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
    class rect_calculator;

    // On-chip Calibration manager owns the background thread
    // leading the calibration process
    // It is controlled by autocalib_notification_model UI object
    // that invokes the process when needed
    class on_chip_calib_manager : public process_manager
    {
    public:
        on_chip_calib_manager(viewer_model& viewer, std::shared_ptr<subdevice_model> sub,
            device_model& model, device dev)
            : process_manager("On-Chip Calibration"), _model(model),
             _dev(dev), _sub(sub), _viewer(viewer)
        {
                auto dev_name = dev.get_info(RS2_CAMERA_INFO_NAME);
                if (!strcmp(dev_name, "Intel RealSense D415")) { speed = 4; }
        }

        bool allow_calib_keep() const { return true; }

        // Get health number from the calibration summary
        float get_health() const { return _health; }
        float get_health_1() const { return _health_1; }
        float get_health_2() const { return _health_2; }

        // Write new calibration to the device
        void keep();

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
            RS2_CALIB_ACTION_ON_CHIP_OB_CALIB,  // One button On-Chip calibration for both
            RS2_CALIB_ACTION_ON_CHIP_CALIB,     // On-Chip calibration
            RS2_CALIB_ACTION_ON_CHIP_FL_CALIB,  // On-Chip focal length calibration
            RS2_CALIB_ACTION_TARE_CALIB,        // Tare calibration
            RS2_CALIB_ACTION_TARE_GROUND_TRUTH, // Tare ground truth
        };

        auto_calib_action action = RS2_CALIB_ACTION_ON_CHIP_OB_CALIB;
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

        std::shared_ptr<subdevice_model> _sub;

        void calibrate();
        void get_ground_truth();

    private:

        std::vector<uint8_t> safe_send_command(const std::vector<uint8_t>& cmd, const std::string& name);

        rs2::depth_frame fetch_depth_frame(invoker invoke);

        std::pair<float, float> get_depth_metrics(invoker invoke);

        void process_flow(std::function<void()> cleanup, invoker invoke) override;

        float _health = -1.0f;
        float _health_1 = -1.0f;
        float _health_2 = -1.0f;
        device _dev;

        bool _was_streaming = false;
        bool _synchronized = false;
        bool _post_processing = false;
        std::shared_ptr<subdevice_ui_selection> _ui { nullptr };
        bool _in_3d_view = false;
        int _uid = 0;

        viewer_model& _viewer;

        std::vector<uint8_t> _old_calib, _new_calib;
        std::vector<std::pair<float, float>> _metrics;
        device_model& _model;

        bool _restored = true;

        void stop_viewer(invoker invoke);
        bool start_viewer(int w, int h, int fps, invoker invoke);
        void try_start_viewer(int w, int h, int fps, invoker invoke);
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
}
