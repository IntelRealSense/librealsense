// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <chrono>
#include <atomic>

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
        cah_model(device_model & dev_model, viewer_model& viewer);

        bool prompt_trigger_popup(ux_window& window, std::string& error_message);
        bool prompt_reset_popup(ux_window& window, std::string& error_message);
    private:

        device_model & _dev_model;
        viewer_model& _viewer;
        enum class model_state_type { TRIGGER_MODAL, PROCESS_MODAL };
        std::atomic<model_state_type> _state; // will be set from a different thread callback function
        bool _process_started;
        timeout _process_timeout;
    };
}
