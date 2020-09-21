// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "cah-model.h"
#include "model-views.h"
 
using namespace rs2;

// This variable is global for protecting the case when the callback will be called when the device model no longer exist.
// TODO: Refactor for handling multiple L515 devices support.
static std::atomic<rs2_calibration_status> global_calib_status; 

cah_model::cah_model(device_model & dev_model, viewer_model& viewer) :
    _dev_model(dev_model),
    _viewer(viewer),
    _state(model_state_type::TRIGGER_MODAL),
    _process_started(false), 
    _process_timeout(std::chrono::seconds(30))
{
    global_calib_status = RS2_CALIBRATION_NOT_NEEDED;

    // Register AC status change callback
    device_calibration dev_cal(dev_model.dev);
    dev_cal.register_calibration_change_callback([&](rs2_calibration_status cal_status)
    {
        global_calib_status = cal_status;
    });
}


bool cah_model::prompt_trigger_popup(ux_window& window, std::string& error_message)
{
    // This process is built from a 2 stages windows, first a yes/no window and then a process window
    bool keep_showing = true;
    bool yes_was_chosen = false;

    switch (_state.load())
    {
    case model_state_type::TRIGGER_MODAL:
    {
        // Make sure the firmware meets the minimal version for Trigger Camera Accuracy features
        const std::string& min_fw_version("1.5.0.0");
        auto fw_upgrade_needed = is_upgradeable(_dev_model.dev.get_info(rs2_camera_info::RS2_CAMERA_INFO_FIRMWARE_VERSION), min_fw_version);
        bool is_depth_streaming = std::any_of(_dev_model.subdevices.begin(), _dev_model.subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm) { return sm->streaming && sm->s->as<depth_sensor>(); });
        bool is_color_streaming = std::any_of(_dev_model.subdevices.begin(), _dev_model.subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm) { return sm->streaming && sm->s->as<color_sensor>(); });
        bool auto_cah_is_working = RS2_CALIBRATION_SUCCESSFUL != global_calib_status
                                && RS2_CALIBRATION_FAILED != global_calib_status
                                && RS2_CALIBRATION_NOT_NEEDED != global_calib_status
                                && RS2_CALIBRATION_BAD_CONDITIONS != global_calib_status;

        std::string message_text = "Camera Accuracy Health will ensure you get the highest accuracy from your camera.\n\n"
            "This process may take several minutes and requires special setup to get good results.\n"
            "While it is working, the viewer will not be usable.";

        std::string disable_reason_text;
        if (fw_upgrade_needed)
        {
            disable_reason_text = "Camera Accuracy Health requires a minimal FW version of " + min_fw_version +
                "\n\nPlease update your firmware and try again. ";
        }
        else if (!is_depth_streaming || !is_color_streaming)
        {
            disable_reason_text = "Camera Accuracy Health cannot be triggered : both depth & RGB streams must be active.";
        }
        else if (auto_cah_is_working)
        {
            disable_reason_text = "Camera Accuracy Health is already in progress in the background.\n"
                            "Please try again in a few minutes. ";
        }
        else
        {
            message_text += "\n\nAre you sure you want to continue?";
        }

        bool option_disabled = !is_depth_streaming || !is_color_streaming || auto_cah_is_working || fw_upgrade_needed;
        if (yes_no_dialog("Camera Accuracy Health Trigger", message_text, yes_was_chosen, window, error_message, option_disabled, disable_reason_text))
        {
            if (yes_was_chosen)
            {

                auto itr = std::find_if(_dev_model.subdevices.begin(), _dev_model.subdevices.end(), [](std::shared_ptr<subdevice_model> sub)
                {
                    if (sub->s->as<depth_sensor>())
                        return true;
                    return false;
                });


                if (is_depth_streaming && is_color_streaming && itr != _dev_model.subdevices.end())
                {
                    auto sd = *itr;
                    global_calib_status = RS2_CALIBRATION_RETRY; // To indicate in progress state
                    try
                    {
                        sd->s->set_option(RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH, static_cast<float>(RS2_CAH_TRIGGER_NOW));
                    }
                    catch( std::exception const & e )
                    {
                        error_message = to_string() << "Trigger calibration failure:\n" << e.what();
                        _process_started = false;
                        global_calib_status = RS2_CALIBRATION_FAILED;
                        return false;
                    }
                    
                    _state = model_state_type::PROCESS_MODAL;
                    // We switch to process state without a guarantee that the process really started,
                    // Set a timeout to make sure if it is not started we will allow closing the window.
                    _process_timeout.start();

                }
            }
            else
            {
                keep_showing = false;
            }
        }
    }
    break;

    case model_state_type::PROCESS_MODAL:
    {
        if (!_process_started)
        {
            // Indication of calibration process start
            _process_started = global_calib_status == RS2_CALIBRATION_TRIGGERED
                            || global_calib_status == RS2_CALIBRATION_SPECIAL_FRAME
                            || global_calib_status == RS2_CALIBRATION_STARTED;
        }

        bool process_finished = global_calib_status == RS2_CALIBRATION_SUCCESSFUL
                             || global_calib_status == RS2_CALIBRATION_FAILED
                             || global_calib_status == RS2_CALIBRATION_NOT_NEEDED
                             || global_calib_status == RS2_CALIBRATION_BAD_CONDITIONS;

        static std::map<rs2_calibration_status, std::string> status_map{
            {RS2_CALIBRATION_TRIGGERED      , "In Progress" },
            {RS2_CALIBRATION_SPECIAL_FRAME  , "In Progress" },
            {RS2_CALIBRATION_STARTED        , "In Progress" },
            {RS2_CALIBRATION_NOT_NEEDED     , "Ended" },
            {RS2_CALIBRATION_SUCCESSFUL     , "Ended Successfully" },
            {RS2_CALIBRATION_BAD_CONDITIONS , "Invalid Conditions" },
            {RS2_CALIBRATION_RETRY          , "In Progress" },
            {RS2_CALIBRATION_FAILED         , "Ended With Failure" },
            {RS2_CALIBRATION_SCENE_INVALID  , "In Progress" },
            {RS2_CALIBRATION_BAD_RESULT     , "In Progress" } };

        rs2_calibration_status calibration_status = global_calib_status;

        // We don't know if AC really started working so we add a timeout for not blocking the viewer forever.
        if (!_process_started)
        {
            if (_process_timeout.has_expired())
            {
                process_finished = true; // on timeout display failure and allow closing the window
                calibration_status = RS2_CALIBRATION_FAILED;
            }
        }

        const std::string & message = process_finished ? "                                                               " :
                                                         "Camera Accuracy Health is In progress, this may take a while...";
        if (status_dialog("Camera Accuracy Health Status", message, status_map[calibration_status], process_finished, window))
        {
            keep_showing = false;
        }
    }
    break;
    default:
        break;

    }

    //reset internal elements for next process
    if (!keep_showing)
    {
        _state = model_state_type::TRIGGER_MODAL;
        _process_started = false;
    }
    return keep_showing;
}

bool cah_model::prompt_reset_popup(ux_window& window, std::string& error_message)
{
    bool keep_showing = true;
    bool yes_was_chosen = false;

    std::string message_text("This will reset the camera settings to their factory-calibrated state.\nYou will lose any improvements made with Camera Accuracy Health.\n\n Are you sure?");
    if (yes_no_dialog("Camera Accuracy Health Reset", message_text, yes_was_chosen, window, error_message))
    {
        if (yes_was_chosen)
        {
            auto itr = std::find_if(_dev_model.subdevices.begin(), _dev_model.subdevices.end(), [](std::shared_ptr<subdevice_model> sub)
            {
                if (sub->s->as<depth_sensor>())
                    return true;
                return false;
            });


            if (itr != _dev_model.subdevices.end())
            {
                auto sd = *itr;
                // Trigger CAH process
                try
                {
                    sd->s->set_option(RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH, 1.0f);
                }
                catch (std::exception const & e)
                {
                    error_message = to_string() << "Calibration reset failure:\n" << e.what();
                }
            }
        }
        keep_showing = false;
    }

    return keep_showing;
}
