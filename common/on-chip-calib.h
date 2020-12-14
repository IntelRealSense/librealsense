// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "notifications.h"
#include "../src/concurrency.h"

#include <random>

namespace rs2
{
    class viewer_model;
    class subdevice_model;
    struct subdevice_ui_selection;
    class tare_ground_truth_calculator;

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

    // Class for calculating the tare ground with the specific target
    class tare_ground_truth_calculator
    {
    public:
        tare_ground_truth_calculator(int width, int height, float focal_length, float target_width, float target_height);
        virtual ~tare_ground_truth_calculator();

        int calculate(const uint8_t* img, float& ground_truth); // return 0 if the target is not in the center, 1 if found, 2 if found and ground truth updated

        tare_ground_truth_calculator(const tare_ground_truth_calculator&) = delete;
        tare_ground_truth_calculator& operator=(const tare_ground_truth_calculator&) = delete;

        tare_ground_truth_calculator(const tare_ground_truth_calculator&&) = delete;
        tare_ground_truth_calculator& operator=(const tare_ground_truth_calculator&&) = delete;

    private:
        void normalize(const uint8_t* img);
        void calculate_ncc();
        bool find_corners();
        void refine_corners();

        int smooth_corner_locations(float& ground_truth);
        void calculate_ground_truth(float& ground_truth);

        void minimize_x(const double* p, int s, double* f, double& x);
        void minimize_y(const double* p, int s, double* f, double& y);
        double subpixel_agj(double* f, int s);

    public:
        static const int _frame_num = 25;

    private:
        const int _tsize = 28;
        const int _htsize = _tsize >> 1;
        const int _tsize2 = _tsize * _tsize;
        std::vector<double> _imgt;

        const std::vector<double> _template
        {
            -0.02870443, -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02812543, -0.02783583, -0.02769113, -0.02725683, -0.02696723, -0.02667773, -0.02624343, -0.02609863, -0.02595393, -0.02580913, -0.02595393, -0.02609863, -0.02624343, -0.02667773, -0.02696723, -0.02725683, -0.02769113, -0.02783583, -0.02812543, -0.02827013, -0.02841493, -0.02855973, -0.02855973,
            -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02769113, -0.02740153, -0.02682253, -0.02624343, -0.02566433, -0.02508533, -0.02465103, -0.02421673, -0.02392713, -0.02378243, -0.02392713, -0.02421673, -0.02465103, -0.02508533, -0.02566433, -0.02624343, -0.02682253, -0.02740153, -0.02769113, -0.02798063, -0.02827013, -0.02841493, -0.02855973,
            -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02711203, -0.02638823, -0.02566433, -0.02479573, -0.02378243, -0.02276903, -0.02190043, -0.02117663, -0.02074233, -0.02059753, -0.02074233, -0.02117663, -0.02190043, -0.02276903, -0.02378243, -0.02479573, -0.02566433, -0.02638823, -0.02711203, -0.02754633, -0.02798063, -0.02827013, -0.02841493,
            -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02508533, -0.02392713, -0.02247953, -0.02088703, -0.01929463, -0.01799173, -0.01683363, -0.01610973, -0.01582023, -0.01610973, -0.01683363, -0.01799173, -0.01929463, -0.02088703, -0.02247953, -0.02392713, -0.02508533, -0.02609863, -0.02696723, -0.02754633, -0.02798063, -0.02827013,
            -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02479573, -0.02320333, -0.02132133, -0.01914983, -0.01683363, -0.01451733, -0.01234583, -0.01060863, -0.00945043, -0.00916093, -0.00945043, -0.01060863, -0.01234583, -0.01451733, -0.01683363, -0.01914983, -0.02132133, -0.02320333, -0.02479573, -0.02609863, -0.02696723, -0.02754633, -0.02798063,
            -0.02812543, -0.02769113, -0.02711203, -0.02609863, -0.02479573, -0.02305853, -0.02074233, -0.01799173, -0.01480683, -0.01133243, -0.00785803, -0.00481793, -0.00221213, -0.00061973, -0.00004063, -0.00061973, -0.00221213, -0.00481793, -0.00785803, -0.01133243, -0.01480683, -0.01799173, -0.02074233, -0.02305853, -0.02479573, -0.02609863, -0.02711203, -0.02769113,
            -0.02783583, -0.02740153, -0.02638823, -0.02508533, -0.02320333, -0.02074233, -0.01755743, -0.01364873, -0.00916093, -0.00423883,  0.00053847,  0.00488147,  0.00850057,  0.01081687,  0.01154077,  0.01081687,  0.00850057,  0.00488147,  0.00053847, -0.00423883, -0.00916093, -0.01364873, -0.01755743, -0.02074233, -0.02320333, -0.02508533, -0.02638823, -0.02740153,
            -0.02769113, -0.02682253, -0.02566433, -0.02392713, -0.02132133, -0.01799173, -0.01364873, -0.00829233, -0.00221213,  0.00430237,  0.01081687,  0.01660757,  0.02138487,  0.02456977,  0.02558307,  0.02456977,  0.02138487,  0.01660757,  0.01081687,  0.00430237, -0.00221213, -0.00829233, -0.01364873, -0.01799173, -0.02132133, -0.02392713, -0.02566433, -0.02682253,
            -0.02725683, -0.02624343, -0.02479573, -0.02247953, -0.01914983, -0.01480683, -0.00916093, -0.00221213,  0.00560527,  0.01400177,  0.02239827,  0.03021567,  0.03629587,  0.04034937,  0.04179697,  0.04034937,  0.03629587,  0.03021567,  0.02239827,  0.01400177,  0.00560527, -0.00221213, -0.00916093, -0.01480683, -0.01914983, -0.02247953, -0.02479573, -0.02624343,
            -0.02696723, -0.02566433, -0.02378243, -0.02088703, -0.01683363, -0.01133243, -0.00423883,  0.00430237,  0.01400177,  0.02456977,  0.03499297,  0.04469237,  0.05236497,  0.05743187,  0.05916907,  0.05743187,  0.05236497,  0.04469237,  0.03499297,  0.02456977,  0.01400177,  0.00430237, -0.00423883, -0.01133243, -0.01683363, -0.02088703, -0.02378243, -0.02566433,
            -0.02667773, -0.02508533, -0.02276903, -0.01929463, -0.01451733, -0.00785803,  0.00053847,  0.01081687,  0.02239827,  0.03499297,  0.04758767,  0.05916907,  0.06828937,  0.07436957,  0.07639627,  0.07436957,  0.06828937,  0.05916907,  0.04758767,  0.03499297,  0.02239827,  0.01081687,  0.00053847, -0.00785803, -0.01451733, -0.01929463, -0.02276903, -0.02508533,
            -0.02624343, -0.02465103, -0.02190043, -0.01799173, -0.01234583, -0.00481793,  0.00488147,  0.01660757,  0.03021567,  0.04469237,  0.05916907,  0.07234287,  0.08291078,  0.08985968,  0.09217588,  0.08985968,  0.08291078,  0.07234287,  0.05916907,  0.04469237,  0.03021567,  0.01660757,  0.00488147, -0.00481793, -0.01234583, -0.01799173, -0.02190043, -0.02465103,
            -0.02609863, -0.02421673, -0.02117663, -0.01683363, -0.01060863, -0.00221213,  0.00850057,  0.02138487,  0.03629587,  0.05236497,  0.06828937,  0.08291078,  0.09463698,  0.10230958,  0.10491538,  0.10230958,  0.09463698,  0.08291078,  0.06828937,  0.05236497,  0.03629587,  0.02138487,  0.00850057, -0.00221213, -0.01060863, -0.01683363, -0.02117663, -0.02421673,
            -0.02595393, -0.02392713, -0.02074233, -0.01610973, -0.00945043, -0.00061973,  0.01081687,  0.02456977,  0.04034937,  0.05743187,  0.07436957,  0.08985968,  0.10230958,  0.11041658,  0.11316708,  0.11041658,  0.10230958,  0.08985968,  0.07436957,  0.05743187,  0.04034937,  0.02456977,  0.01081687, -0.00061973, -0.00945043, -0.01610973, -0.02074233, -0.02392713,
            -0.02580913, -0.02378243, -0.02059753, -0.01582023, -0.00916093, -0.00004063,  0.01154077,  0.02558307,  0.04179697,  0.05916907,  0.07639627,  0.09217588,  0.10491538,  0.11316708,  0.11606248,  0.11316708,  0.10491538,  0.09217588,  0.07639627,  0.05916907,  0.04179697,  0.02558307,  0.01154077, -0.00004063, -0.00916093, -0.01582023, -0.02059753, -0.02378243,
            -0.02595393, -0.02392713, -0.02074233, -0.01610973, -0.00945043, -0.00061973,  0.01081687,  0.02456977,  0.04034937,  0.05743187,  0.07436957,  0.08985968,  0.10230958,  0.11041658,  0.11316708,  0.11041658,  0.10230958,  0.08985968,  0.07436957,  0.05743187,  0.04034937,  0.02456977,  0.01081687, -0.00061973, -0.00945043, -0.01610973, -0.02074233, -0.02392713,
            -0.02609863, -0.02421673, -0.02117663, -0.01683363, -0.01060863, -0.00221213,  0.00850057,  0.02138487,  0.03629587,  0.05236497,  0.06828937,  0.08291078,  0.09463698,  0.10230958,  0.10491538,  0.10230958,  0.09463698,  0.08291078,  0.06828937,  0.05236497,  0.03629587,  0.02138487,  0.00850057, -0.00221213, -0.01060863, -0.01683363, -0.02117663, -0.02421673,
            -0.02624343, -0.02465103, -0.02190043, -0.01799173, -0.01234583, -0.00481793,  0.00488147,  0.01660757,  0.03021567,  0.04469237,  0.05916907,  0.07234287,  0.08291078,  0.08985968,  0.09217588,  0.08985968,  0.08291078,  0.07234287,  0.05916907,  0.04469237,  0.03021567,  0.01660757,  0.00488147, -0.00481793, -0.01234583, -0.01799173, -0.02190043, -0.02465103,
            -0.02667773, -0.02508533, -0.02276903, -0.01929463, -0.01451733, -0.00785803,  0.00053847,  0.01081687,  0.02239827,  0.03499297,  0.04758767,  0.05916907,  0.06828937,  0.07436957,  0.07639627,  0.07436957,  0.06828937,  0.05916907,  0.04758767,  0.03499297,  0.02239827,  0.01081687,  0.00053847, -0.00785803, -0.01451733, -0.01929463, -0.02276903, -0.02508533,
            -0.02696723, -0.02566433, -0.02378243, -0.02088703, -0.01683363, -0.01133243, -0.00423883,  0.00430237,  0.01400177,  0.02456977,  0.03499297,  0.04469237,  0.05236497,  0.05743187,  0.05916907,  0.05743187,  0.05236497,  0.04469237,  0.03499297,  0.02456977,  0.01400177,  0.00430237, -0.00423883, -0.01133243, -0.01683363, -0.02088703, -0.02378243, -0.02566433,
            -0.02725683, -0.02624343, -0.02479573, -0.02247953, -0.01914983, -0.01480683, -0.00916093, -0.00221213,  0.00560527,  0.01400177,  0.02239827,  0.03021567,  0.03629587,  0.04034937,  0.04179697,  0.04034937,  0.03629587,  0.03021567,  0.02239827,  0.01400177,  0.00560527, -0.00221213, -0.00916093, -0.01480683, -0.01914983, -0.02247953, -0.02479573, -0.02624343,
            -0.02769113, -0.02682253, -0.02566433, -0.02392713, -0.02132133, -0.01799173, -0.01364873, -0.00829233, -0.00221213,  0.00430237,  0.01081687,  0.01660757,  0.02138487,  0.02456977,  0.02558307,  0.02456977,  0.02138487,  0.01660757,  0.01081687,  0.00430237, -0.00221213, -0.00829233, -0.01364873, -0.01799173, -0.02132133, -0.02392713, -0.02566433, -0.02682253,
            -0.02783583, -0.02740153, -0.02638823, -0.02508533, -0.02320333, -0.02074233, -0.01755743, -0.01364873, -0.00916093, -0.00423883,  0.00053847,  0.00488147,  0.00850057,  0.01081687,  0.01154077,  0.01081687,  0.00850057,  0.00488147,  0.00053847, -0.00423883, -0.00916093, -0.01364873, -0.01755743, -0.02074233, -0.02320333, -0.02508533, -0.02638823, -0.02740153,
            -0.02812543, -0.02769113, -0.02711203, -0.02609863, -0.02479573, -0.02305853, -0.02074233, -0.01799173, -0.01480683, -0.01133243, -0.00785803, -0.00481793, -0.00221213, -0.00061973, -0.00004063, -0.00061973, -0.00221213, -0.00481793, -0.00785803, -0.01133243, -0.01480683, -0.01799173, -0.02074233, -0.02305853, -0.02479573, -0.02609863, -0.02711203, -0.02769113,
            -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02479573, -0.02320333, -0.02132133, -0.01914983, -0.01683363, -0.01451733, -0.01234583, -0.01060863, -0.00945043, -0.00916093, -0.00945043, -0.01060863, -0.01234583, -0.01451733, -0.01683363, -0.01914983, -0.02132133, -0.02320333, -0.02479573, -0.02609863, -0.02696723, -0.02754633, -0.02798063,
            -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02508533, -0.02392713, -0.02247953, -0.02088703, -0.01929463, -0.01799173, -0.01683363, -0.01610973, -0.01582023, -0.01610973, -0.01683363, -0.01799173, -0.01929463, -0.02088703, -0.02247953, -0.02392713, -0.02508533, -0.02609863, -0.02696723, -0.02754633, -0.02798063, -0.02827013,
            -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02711203, -0.02638823, -0.02566433, -0.02479573, -0.02378243, -0.02276903, -0.02190043, -0.02117663, -0.02074233, -0.02059753, -0.02074233, -0.02117663, -0.02190043, -0.02276903, -0.02378243, -0.02479573, -0.02566433, -0.02638823, -0.02711203, -0.02754633, -0.02798063, -0.02827013, -0.02841493,
            -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02769113, -0.02740153, -0.02682253, -0.02624343, -0.02566433, -0.02508533, -0.02465103, -0.02421673, -0.02392713, -0.02378243, -0.02392713, -0.02421673, -0.02465103, -0.02508533, -0.02566433, -0.02624343, -0.02682253, -0.02740153, -0.02769113, -0.02798063, -0.02827013, -0.02841493, -0.02855973,
        };

        double _target_fw = 0.0;
        double _target_fh = 0.0;

        double _thresh = 0.7;
        int _patch_size = 20;

        std::vector<double> _img;
        std::vector<double> _ncc;
        int _width = 0;
        int _height = 0;
        int _size = 0;

        int _wt = 0;
        int _ht = 0;
        int _hwidth;
        int _hheight;

        template <typename T>
        struct point
        {
            T x;
            T y;
        };

        std::array<point<double>[4], _frame_num> _corners;
        int _corners_idx = 0;
        int _corners_num = 0;
        const int _reset_limit = 10;

        point<int> _pts[4];
        std::vector<double> _buf;
        bool _corners_found = false;

        std::mutex _mtx;
    };
}
