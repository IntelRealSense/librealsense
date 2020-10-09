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

        // Write new calibration to the device
        void keep();

        // Restore Viewer UI to how it was before auto-calib
        void restore_workspace(invoker invoke);
        
        // Ask the firmware to use one of the before/after calibration tables
        void apply_calib(bool old);

        // Get depth metrics for before/after calibration tables
        std::pair<float, float> get_metric(bool use_new);

        void update_last_used();

        float ground_truth = 2500.0f;
        int average_step_count = 20;
        int step_count = 20;
        int accuracy = 2;
        int speed = 3;
        bool intrinsic_scan = true;
        bool apply_preset = true;

        enum auto_calib_action
        {
            RS2_CALIB_ACTION_ON_CHIP_CALIB,     // On-Chip calibration
            RS2_CALIB_ACTION_TARE_CALIB,        // Tare calibration
            RS2_CALIB_ACTION_TARE_GROUND_TRUTH, // Tare ground truth
            RS2_CALIB_ACTION_ON_CHIP_FL_CALIB,  // On-Chip calibration
        };

        auto_calib_action action = RS2_CALIB_ACTION_ON_CHIP_CALIB;
        float laser_status_prev = 0.0f;
        int adjust_both_sides = 0;

        std::shared_ptr<subdevice_model> _sub;

        void calibrate();
        void get_ground_truth();

    private:

        std::vector<uint8_t> safe_send_command(const std::vector<uint8_t>& cmd, const std::string& name);

        rs2::depth_frame fetch_depth_frame(invoker invoke);

        std::pair<float, float> get_depth_metrics(invoker invoke);

        void process_flow(std::function<void()> cleanup, invoker invoke) override;

        float _health = -1.0f;
        device _dev;

        bool _was_streaming = false;
        bool _synchronized = false;
        bool _post_processing = false;
        std::shared_ptr<subdevice_ui_selection> _ui { nullptr };
        bool _in_3d_view = false;

        viewer_model& _viewer;

        std::vector<uint8_t> _old_calib, _new_calib;
        std::vector<std::pair<float, float>> _metrics;
        device_model& _model;

        bool _restored = true;

        void stop_viewer(invoker invoke);
        void start_viewer(int w, int h, int fps, invoker invoke);
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
            RS2_CALIB_STATE_FL_SELF_INPUT,       // Collect input parameters for focal length on-chip self calibration
            RS2_CALIB_STATE_FL_CALIB_IN_PROCESS, // Focal length calibration in process... Shows progressbar
            RS2_CALIB_STATE_FL_CALIB_COMPLETE,   // Focal length calibration, show before/after toggle and metrics
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
        void normalize_0(const uint8_t* img);
        void normalize_1(const uint8_t* img);
        void normalize_2(const uint8_t* img);
        void normalize_3(const uint8_t* img);
        void find_corner(float thresh, int q, int patch_size);

        void pre_calculate(const uint8_t* img, float thresh, int patch_size);
        int run(float& ground_truth);
        float calculate_depth();

        void minimize_x(const float* p, int s, float* f, float& x);
        void minimize_y(const float* p, int s, float* f, float& y);
        bool fit_parabola(float x1, float y1, float x2, float y2, float x3, float y3, float& x0, float& y0);

    public:
        static const int _frame_num = 50;

    private:
        const float _thresh = 0.7f;
        const int _patch_size = 20;

        const int _tsize = 28;
        const int _htsize = _tsize >> 1;
        const int _tsize2 = _tsize * _tsize;
        std::vector<float> _imgt[4];

        const std::vector<float> _template
        {
            -0.0287044f, -0.0285597f, -0.0285597f, -0.0284149f, -0.0282701f, -0.0281254f, -0.0278358f, -0.0276911f, -0.0272568f, -0.0269672f, -0.0266777f, -0.0262434f, -0.0260986f, -0.0259539f, -0.0258091f, -0.0259539f, -0.0260986f, -0.0262434f, -0.0266777f, -0.0269672f, -0.0272568f, -0.0276911f, -0.0278358f, -0.0281254f, -0.0282701f, -0.0284149f, -0.0285597f, -0.0285597f,
            -0.0285597f, -0.0285597f, -0.0284149f, -0.0282701f, -0.0279806f, -0.0276911f, -0.0274015f, -0.0268225f, -0.0262434f, -0.0256643f, -0.0250853f, -0.0246510f, -0.0242167f, -0.0239271f, -0.0237824f, -0.0239271f, -0.0242167f, -0.0246510f, -0.0250853f, -0.0256643f, -0.0262434f, -0.0268225f, -0.0274015f, -0.0276911f, -0.0279806f, -0.0282701f, -0.0284149f, -0.0285597f,
            -0.0285597f, -0.0284149f, -0.0282701f, -0.0279806f, -0.0275463f, -0.0271120f, -0.0263882f, -0.0256643f, -0.0247957f, -0.0237824f, -0.0227690f, -0.0219004f, -0.0211766f, -0.0207423f, -0.0205975f, -0.0207423f, -0.0211766f, -0.0219004f, -0.0227690f, -0.0237824f, -0.0247957f, -0.0256643f, -0.0263882f, -0.0271120f, -0.0275463f, -0.0279806f, -0.0282701f, -0.0284149f,
            -0.0284149f, -0.0282701f, -0.0279806f, -0.0275463f, -0.0269672f, -0.0260986f, -0.0250853f, -0.0239271f, -0.0224795f, -0.0208870f, -0.0192946f, -0.0179917f, -0.0168336f, -0.0161097f, -0.0158202f, -0.0161097f, -0.0168336f, -0.0179917f, -0.0192946f, -0.0208870f, -0.0224795f, -0.0239271f, -0.0250853f, -0.0260986f, -0.0269672f, -0.0275463f, -0.0279806f, -0.0282701f,
            -0.0282701f, -0.0279806f, -0.0275463f, -0.0269672f, -0.0260986f, -0.0247957f, -0.0232033f, -0.0213213f, -0.0191498f, -0.0168336f, -0.0145173f, -0.0123458f, -0.0106086f, -0.0094504f, -0.0091609f, -0.0094504f, -0.0106086f, -0.0123458f, -0.0145173f, -0.0168336f, -0.0191498f, -0.0213213f, -0.0232033f, -0.0247957f, -0.0260986f, -0.0269672f, -0.0275463f, -0.0279806f,
            -0.0281254f, -0.0276911f, -0.0271120f, -0.0260986f, -0.0247957f, -0.0230585f, -0.0207423f, -0.0179917f, -0.0148068f, -0.0113324f, -0.0078580f, -0.0048179f, -0.0022121f, -0.0006197f, -0.0000406f, -0.0006197f, -0.0022121f, -0.0048179f, -0.0078580f, -0.0113324f, -0.0148068f, -0.0179917f, -0.0207423f, -0.0230585f, -0.0247957f, -0.0260986f, -0.0271120f, -0.0276911f,
            -0.0278358f, -0.0274015f, -0.0263882f, -0.0250853f, -0.0232033f, -0.0207423f, -0.0175574f, -0.0136487f, -0.0091609f, -0.0042388f,  0.0005385f,  0.0048815f,  0.0085006f,  0.0108169f,  0.0115408f,  0.0108169f,  0.0085006f,  0.0048815f,  0.0005385f, -0.0042388f, -0.0091609f, -0.0136487f, -0.0175574f, -0.0207423f, -0.0232033f, -0.0250853f, -0.0263882f, -0.0274015f,
            -0.0276911f, -0.0268225f, -0.0256643f, -0.0239271f, -0.0213213f, -0.0179917f, -0.0136487f, -0.0082923f, -0.0022121f,  0.0043024f,  0.0108169f,  0.0166076f,  0.0213849f,  0.0245698f,  0.0255831f,  0.0245698f,  0.0213849f,  0.0166076f,  0.0108169f,  0.0043024f, -0.0022121f, -0.0082923f, -0.0136487f, -0.0179917f, -0.0213213f, -0.0239271f, -0.0256643f, -0.0268225f,
            -0.0272568f, -0.0262434f, -0.0247957f, -0.0224795f, -0.0191498f, -0.0148068f, -0.0091609f, -0.0022121f,  0.0056053f,  0.0140018f,  0.0223983f,  0.0302157f,  0.0362959f,  0.0403494f,  0.0417970f,  0.0403494f,  0.0362959f,  0.0302157f,  0.0223983f,  0.0140018f,  0.0056053f, -0.0022121f, -0.0091609f, -0.0148068f, -0.0191498f, -0.0224795f, -0.0247957f, -0.0262434f,
            -0.0269672f, -0.0256643f, -0.0237824f, -0.0208870f, -0.0168336f, -0.0113324f, -0.0042388f,  0.0043024f,  0.0140018f,  0.0245698f,  0.0349930f,  0.0446924f,  0.0523650f,  0.0574319f,  0.0591691f,  0.0574319f,  0.0523650f,  0.0446924f,  0.0349930f,  0.0245698f,  0.0140018f,  0.0043024f, -0.0042388f, -0.0113324f, -0.0168336f, -0.0208870f, -0.0237824f, -0.0256643f,
            -0.0266777f, -0.0250853f, -0.0227690f, -0.0192946f, -0.0145173f, -0.0078580f,  0.0005385f,  0.0108169f,  0.0223983f,  0.0349930f,  0.0475877f,  0.0591691f,  0.0682894f,  0.0743696f,  0.0763963f,  0.0743696f,  0.0682894f,  0.0591691f,  0.0475877f,  0.0349930f,  0.0223983f,  0.0108169f,  0.0005385f, -0.0078580f, -0.0145173f, -0.0192946f, -0.0227690f, -0.0250853f,
            -0.0262434f, -0.0246510f, -0.0219004f, -0.0179917f, -0.0123458f, -0.0048179f,  0.0048815f,  0.0166076f,  0.0302157f,  0.0446924f,  0.0591691f,  0.0723429f,  0.0829108f,  0.0898597f,  0.0921759f,  0.0898597f,  0.0829108f,  0.0723429f,  0.0591691f,  0.0446924f,  0.0302157f,  0.0166076f,  0.0048815f, -0.0048179f, -0.0123458f, -0.0179917f, -0.0219004f, -0.0246510f,
            -0.0260986f, -0.0242167f, -0.0211766f, -0.0168336f, -0.0106086f, -0.0022121f,  0.0085006f,  0.0213849f,  0.0362959f,  0.0523650f,  0.0682894f,  0.0829108f,  0.0946370f,  0.1023096f,  0.1049154f,  0.1023096f,  0.0946370f,  0.0829108f,  0.0682894f,  0.0523650f,  0.0362959f,  0.0213849f,  0.0085006f, -0.0022121f, -0.0106086f, -0.0168336f, -0.0211766f, -0.0242167f,
            -0.0259539f, -0.0239271f, -0.0207423f, -0.0161097f, -0.0094504f, -0.0006197f,  0.0108169f,  0.0245698f,  0.0403494f,  0.0574319f,  0.0743696f,  0.0898597f,  0.1023096f,  0.1104166f,  0.1131671f,  0.1104166f,  0.1023096f,  0.0898597f,  0.0743696f,  0.0574319f,  0.0403494f,  0.0245698f,  0.0108169f, -0.0006197f, -0.0094504f, -0.0161097f, -0.0207423f, -0.0239271f,
            -0.0258091f, -0.0237824f, -0.0205975f, -0.0158202f, -0.0091609f, -0.0000406f,  0.0115408f,  0.0255831f,  0.0417970f,  0.0591691f,  0.0763963f,  0.0921759f,  0.1049154f,  0.1131671f,  0.1160625f,  0.1131671f,  0.1049154f,  0.0921759f,  0.0763963f,  0.0591691f,  0.0417970f,  0.0255831f,  0.0115408f, -0.0000406f, -0.0091609f, -0.0158202f, -0.0205975f, -0.0237824f,
            -0.0259539f, -0.0239271f, -0.0207423f, -0.0161097f, -0.0094504f, -0.0006197f,  0.0108169f,  0.0245698f,  0.0403494f,  0.0574319f,  0.0743696f,  0.0898597f,  0.1023096f,  0.1104166f,  0.1131671f,  0.1104166f,  0.1023096f,  0.0898597f,  0.0743696f,  0.0574319f,  0.0403494f,  0.0245698f,  0.0108169f, -0.0006197f, -0.0094504f, -0.0161097f, -0.0207423f, -0.0239271f,
            -0.0260986f, -0.0242167f, -0.0211766f, -0.0168336f, -0.0106086f, -0.0022121f,  0.0085006f,  0.0213849f,  0.0362959f,  0.0523650f,  0.0682894f,  0.0829108f,  0.0946370f,  0.1023096f,  0.1049154f,  0.1023096f,  0.0946370f,  0.0829108f,  0.0682894f,  0.0523650f,  0.0362959f,  0.0213849f,  0.0085006f, -0.0022121f, -0.0106086f, -0.0168336f, -0.0211766f, -0.0242167f,
            -0.0262434f, -0.0246510f, -0.0219004f, -0.0179917f, -0.0123458f, -0.0048179f,  0.0048815f,  0.0166076f,  0.0302157f,  0.0446924f,  0.0591691f,  0.0723429f,  0.0829108f,  0.0898597f,  0.0921759f,  0.0898597f,  0.0829108f,  0.0723429f,  0.0591691f,  0.0446924f,  0.0302157f,  0.0166076f,  0.0048815f, -0.0048179f, -0.0123458f, -0.0179917f, -0.0219004f, -0.0246510f,
            -0.0266777f, -0.0250853f, -0.0227690f, -0.0192946f, -0.0145173f, -0.0078580f,  0.0005385f,  0.0108169f,  0.0223983f,  0.0349930f,  0.0475877f,  0.0591691f,  0.0682894f,  0.0743696f,  0.0763963f,  0.0743696f,  0.0682894f,  0.0591691f,  0.0475877f,  0.0349930f,  0.0223983f,  0.0108169f,  0.0005385f, -0.0078580f, -0.0145173f, -0.0192946f, -0.0227690f, -0.0250853f,
            -0.0269672f, -0.0256643f, -0.0237824f, -0.0208870f, -0.0168336f, -0.0113324f, -0.0042388f,  0.0043024f,  0.0140018f,  0.0245698f,  0.0349930f,  0.0446924f,  0.0523650f,  0.0574319f,  0.0591691f,  0.0574319f,  0.0523650f,  0.0446924f,  0.0349930f,  0.0245698f,  0.0140018f,  0.0043024f, -0.0042388f, -0.0113324f, -0.0168336f, -0.0208870f, -0.0237824f, -0.0256643f,
            -0.0272568f, -0.0262434f, -0.0247957f, -0.0224795f, -0.0191498f, -0.0148068f, -0.0091609f, -0.0022121f,  0.0056053f,  0.0140018f,  0.0223983f,  0.0302157f,  0.0362959f,  0.0403494f,  0.0417970f,  0.0403494f,  0.0362959f,  0.0302157f,  0.0223983f,  0.0140018f,  0.0056053f, -0.0022121f, -0.0091609f, -0.0148068f, -0.0191498f, -0.0224795f, -0.0247957f, -0.0262434f,
            -0.0276911f, -0.0268225f, -0.0256643f, -0.0239271f, -0.0213213f, -0.0179917f, -0.0136487f, -0.0082923f, -0.0022121f,  0.0043024f,  0.0108169f,  0.0166076f,  0.0213849f,  0.0245698f,  0.0255831f,  0.0245698f,  0.0213849f,  0.0166076f,  0.0108169f,  0.0043024f, -0.0022121f, -0.0082923f, -0.0136487f, -0.0179917f, -0.0213213f, -0.0239271f, -0.0256643f, -0.0268225f,
            -0.0278358f, -0.0274015f, -0.0263882f, -0.0250853f, -0.0232033f, -0.0207423f, -0.0175574f, -0.0136487f, -0.0091609f, -0.0042388f,  0.0005385f,  0.0048815f,  0.0085006f,  0.0108169f,  0.0115408f,  0.0108169f,  0.0085006f,  0.0048815f,  0.0005385f, -0.0042388f, -0.0091609f, -0.0136487f, -0.0175574f, -0.0207423f, -0.0232033f, -0.0250853f, -0.0263882f, -0.0274015f,
            -0.0281254f, -0.0276911f, -0.0271120f, -0.0260986f, -0.0247957f, -0.0230585f, -0.0207423f, -0.0179917f, -0.0148068f, -0.0113324f, -0.0078580f, -0.0048179f, -0.0022121f, -0.0006197f, -0.0000406f, -0.0006197f, -0.0022121f, -0.0048179f, -0.0078580f, -0.0113324f, -0.0148068f, -0.0179917f, -0.0207423f, -0.0230585f, -0.0247957f, -0.0260986f, -0.0271120f, -0.0276911f,
            -0.0282701f, -0.0279806f, -0.0275463f, -0.0269672f, -0.0260986f, -0.0247957f, -0.0232033f, -0.0213213f, -0.0191498f, -0.0168336f, -0.0145173f, -0.0123458f, -0.0106086f, -0.0094504f, -0.0091609f, -0.0094504f, -0.0106086f, -0.0123458f, -0.0145173f, -0.0168336f, -0.0191498f, -0.0213213f, -0.0232033f, -0.0247957f, -0.0260986f, -0.0269672f, -0.0275463f, -0.0279806f,
            -0.0284149f, -0.0282701f, -0.0279806f, -0.0275463f, -0.0269672f, -0.0260986f, -0.0250853f, -0.0239271f, -0.0224795f, -0.0208870f, -0.0192946f, -0.0179917f, -0.0168336f, -0.0161097f, -0.0158202f, -0.0161097f, -0.0168336f, -0.0179917f, -0.0192946f, -0.0208870f, -0.0224795f, -0.0239271f, -0.0250853f, -0.0260986f, -0.0269672f, -0.0275463f, -0.0279806f, -0.0282701f,
            -0.0285597f, -0.0284149f, -0.0282701f, -0.0279806f, -0.0275463f, -0.0271120f, -0.0263882f, -0.0256643f, -0.0247957f, -0.0237824f, -0.0227690f, -0.0219004f, -0.0211766f, -0.0207423f, -0.0205975f, -0.0207423f, -0.0211766f, -0.0219004f, -0.0227690f, -0.0237824f, -0.0247957f, -0.0256643f, -0.0263882f, -0.0271120f, -0.0275463f, -0.0279806f, -0.0282701f, -0.0284149f,
            -0.0285597f, -0.0285597f, -0.0284149f, -0.0282701f, -0.0279806f, -0.0276911f, -0.0274015f, -0.0268225f, -0.0262434f, -0.0256643f, -0.0250853f, -0.0246510f, -0.0242167f, -0.0239271f, -0.0237824f, -0.0239271f, -0.0242167f, -0.0246510f, -0.0250853f, -0.0256643f, -0.0262434f, -0.0268225f, -0.0274015f, -0.0276911f, -0.0279806f, -0.0282701f, -0.0284149f, -0.0285597f,
        };

        std::vector<float> _imgq[4];
        std::vector<float> _nccq[4];
        int _width = 0;
        int _height = 0;
        int _size = 0;

        int _hwidth;
        int _hheight;
        int _sizeq = 0;

        int _hwt = 0;
        int _hht = 0;

        float _target_fw = 0;
        float _target_fh = 0;

        template <typename T>
        struct point
        {
            T x;
            T y;
        };

        std::array<point<float>[4], _frame_num> _corners;
        int _corners_idx = 0;
        int _corners_num = 0;
        const int _reset_limit = 10;

        point<int> _pts[4];
        std::vector<float> _buf[4];
        bool _corners_found[4];
    };
}
