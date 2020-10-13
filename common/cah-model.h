// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <utilities/time/timer.h>
#include <string>
#include <atomic>

namespace rs2
{
    class ux_window;
    class viewer_model;
    class device_model;

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
        utilities::time::timer _process_timeout;
    };
}
