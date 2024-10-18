// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "notifications.h"
#include <rsutils/concurrency/concurrency.h>
#include "../src/algo.h"

#include <random>
#include <string>

namespace rs2
{
    class viewer_model;
    class subdevice_model;
    struct subdevice_ui_selection;

    // On-chip Calibration manager owns the background thread
    // leading the calibration process
    // It is controlled by autocalib_notification_model UI object
    // that invokes the process when needed
    class d500_on_chip_calib_manager : public process_manager
    {
    public:
        d500_on_chip_calib_manager(viewer_model& viewer, std::shared_ptr<subdevice_model> sub, device_model& model, device dev);

        enum calib_action
        {
            RS2_CALIB_ACTION_ON_CHIP_CALIB,         // On-Chip calibration
            RS2_CALIB_ACTION_ON_CHIP_CALIB_DRY_RUN, // Dry Run
            RS2_CALIB_ACTION_ON_CHIP_CALIB_ABORT    // Abort
        };

        calib_action action = RS2_CALIB_ACTION_ON_CHIP_CALIB;

        std::shared_ptr<subdevice_model> _sub;

        void reset_device() { _dev.hardware_reset(); }
        bool abort();
        void prepare_for_calibration();
        std::string get_device_pid() const;

    private:
        void process_flow(std::function<void()> cleanup, invoker invoke) override;
        std::string convert_action_to_json_string();

        template<class T>
        void set_option_if_needed(T& sensor, rs2_option opt, float required_value);
        device _dev;
        device_model& _model;
    };

    template<class T>
    void d500_on_chip_calib_manager::set_option_if_needed(T& sensor,
        rs2_option opt, float required_value)
    {
        auto curr_value = sensor.get_option(opt);
        if (curr_value != required_value)
        {
            sensor.set_option(opt, required_value);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            curr_value = sensor.get_option(opt);
            if (curr_value != required_value)
            {
                std::stringstream s;
                s << "Could not set " << rs2_option_to_string(opt) << " to " << required_value;
                throw std::runtime_error(s.str().c_str());
            }
        }
    }

    // Auto-calib notification model is managing the UI state-machine
    // controling auto-calibration
    struct d500_autocalib_notification_model : public process_notification_model
    {
        enum auto_calib_ui_state
        {
            RS2_CALIB_STATE_INIT_CALIB,      // First screen
            RS2_CALIB_STATE_FAILED,          // Failed, show _error_message
            RS2_CALIB_STATE_COMPLETE,        // After write, quick blue notification
            RS2_CALIB_STATE_CALIB_IN_PROCESS,// Calibration in process... Shows progressbar
            RS2_CALIB_STATE_INIT_DRY_RUN,
            RS2_CALIB_STATE_ABORT,
            RS2_CALIB_STATE_ABORT_CALLED
        };

        d500_autocalib_notification_model(std::string name, std::shared_ptr<process_manager> manager, bool expaned);

        d500_on_chip_calib_manager& get_manager() { return *std::dynamic_pointer_cast<d500_on_chip_calib_manager>(update_manager); }
        void draw_content(ux_window& win, int x, int y, float t, std::string& error_message) override;
        int calc_height() override;
        void calibration_button(ux_window& win, int x, int y, int bar_width);
        void draw_abort(ux_window& win, int x, int y);
        void update_ui_after_abort_called(ux_window& win, int x, int y);
        void update_ui_on_calibration_complete(ux_window& win, int x, int y);
        void update_ui_on_failure(ux_window& win, int x, int y);
        std::string _error_message = "";
        bool reset_called = false;
        bool _has_abort_succeeded = false;
    };

}
