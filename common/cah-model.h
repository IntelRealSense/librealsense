// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <chrono>
#include <atomic>

#include <librealsense2/rs.hpp>

namespace rs2
{
    class ux_window;
    class viewer_model;
    class device_model;

    // Helper class for setting , starting and checking a timeout.
    // Inner units are [ms] , API units are seconds.
    // This class is not thread safe and should not be passed between threads.
    class timeout
    {
    public:
        timeout() : timeout_delay_ms(0), start_time_ms(0) {};

        // Start timer from current time
        // Can be called with std::chrono::hours/minutes/seconds/milliseconds/microseconds/nanoseconds type
        void start(std::chrono::seconds timeout)
        {
            using namespace std::chrono;
            start_time_ms = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
            timeout_delay_ms = duration_cast<milliseconds>(timeout).count();
        }

        // Check if timeout expired
        bool has_expired() const
        {
            using namespace std::chrono;
            auto curr_time = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
            return curr_time - start_time_ms >= timeout_delay_ms;
        }

    private:
        long long timeout_delay_ms;
        long long start_time_ms;
    };

    class cah_model // CAH = Camera Accuracy Health
    {
    public:
        cah_model() :_state(model_state_type::TRIGGER_MODAL), _calib_status(RS2_CALIBRATION_RETRY),
            _registered_to_callback(false), _process_started(false), _process_timeout()
        {}

        bool prompt_trigger_popup(device_model & dev_model, ux_window& window, viewer_model& viewer, const std::string& error_message);
        bool prompt_reset_popup(device_model & dev_model, ux_window& window, const std::string& error_message);

    private:

        enum class model_state_type { TRIGGER_MODAL, PROCESS_MODAL };
        std::atomic<model_state_type> _state; // will be set from a different thread callback function
        std::atomic<rs2_calibration_status> _calib_status; // will be set from a different thread callback function
        bool _registered_to_callback;
        bool _process_started;
        timeout _process_timeout;
    };
}
