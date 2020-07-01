// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "cah-model.h"
#define GLFW_INCLUDE_NONE
#include "fw-update-helper.h"
#include "model-views.h"

using namespace rs2;

bool cah_model::prompt_trigger_camera_accuracy_health(device_model & dev_model, ux_window& window, viewer_model& viewer, const std::string& error_message)
{
    // This process is built from a 2 stages windows, first a yes/no window and then a process window
    bool keep_showing = true;
    bool yes_was_chosen = false;

    switch (cah_state.load())
    {
    case model_state_type::TRIGGER_MODAL:
    {
        // Make sure the firmware meets the minimal version for Trigger Camera Accuracy features
        const std::string& min_fw_version("1.4.1.0");
        auto fw_upgrade_needed = is_upgradeable(dev_model.dev.get_info(rs2_camera_info::RS2_CAMERA_INFO_FIRMWARE_VERSION), min_fw_version);
        bool is_depth_streaming(false);
        bool is_color_streaming(false);

        std::string message_text = "Camera Accuracy Health will ensure you get the highest accuracy from your camera.\n\n"
            "This process may take several minutes and requires special setup to get good results.\n"
            "While it is working, the viewer will not be usable.\n\n";

        if (fw_upgrade_needed)
        {
            std::string fw_upgrade_message = "Camera Accuracy Health requires a minimal FW version of " + min_fw_version +
                "\n\nPlease update your firmware and try again. ";

            message_text += fw_upgrade_message;
        }
        else
        {
            is_depth_streaming = std::any_of(dev_model.subdevices.begin(), dev_model.subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm) { return sm->streaming && sm->s->as<depth_sensor>(); });
            is_color_streaming = std::any_of(dev_model.subdevices.begin(), dev_model.subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm) { return sm->streaming && sm->s->as<color_sensor>(); });

            if (is_depth_streaming && is_color_streaming)
            {
                message_text += "Are you sure you want to continue?";
            }
            else
            {
                std::string stream_missing_message = "Camera Accuracy Health cannot be triggered : both depth & RGB streams must be active.";
                message_text += stream_missing_message;
            }

        }

        bool option_disabled = !is_depth_streaming || !is_color_streaming || fw_upgrade_needed;
        if (yes_no_dialog("Camera Accuracy Health Trigger", message_text, yes_was_chosen, window, error_message, option_disabled))
        {
            if (yes_was_chosen)
            {

                auto itr = std::find_if(dev_model.subdevices.begin(), dev_model.subdevices.end(), [](std::shared_ptr<subdevice_model> sub)
                {
                    if (sub->s->as<depth_sensor>())
                        return true;
                    return false;
                });


                if (is_depth_streaming && is_color_streaming && itr != dev_model.subdevices.end())
                {
                    auto sd = *itr;
                    sd->s->set_option(RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH, 1.0f);
                    device_calibration dev_cal(dev_model.dev);
                    // Register AC status change callback
                    if (!registered_to_callback)
                    {
                        registered_to_callback = true;
                        cah_state = model_state_type::PROCESS_MODAL;
                        // We switch to process state without a guarantee that the process really started,
                        // Set a timeout to make sure if it is not started we will allow closing the window.
                        cah_process_timeout.start(std::chrono::seconds(30)); 
                        dev_cal.register_calibration_change_callback([&](rs2_calibration_status cal_status)
                        {
                            calib_status = cal_status;
                        });
                    }
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
        if (!process_started)
        {
            // Indication of calibration process start
            process_started = (calib_status == RS2_CALIBRATION_SPECIAL_FRAME || calib_status == RS2_CALIBRATION_STARTED);
        }

        bool process_finished(calib_status == RS2_CALIBRATION_SUCCESSFUL || calib_status == RS2_CALIBRATION_FAILED || calib_status == RS2_CALIBRATION_NOT_NEEDED);

        static std::map<rs2_calibration_status, std::string> status_map{
            {RS2_CALIBRATION_SPECIAL_FRAME  , "In Progress" },
            {RS2_CALIBRATION_STARTED        , "In Progress" },
            {RS2_CALIBRATION_NOT_NEEDED     , "Ended" },
            {RS2_CALIBRATION_SUCCESSFUL     , "Ended Successfully" },
            {RS2_CALIBRATION_RETRY          , "In Progress" },
            {RS2_CALIBRATION_FAILED         , "Ended With Failure" },
            {RS2_CALIBRATION_SCENE_INVALID  , "In Progress" },
            {RS2_CALIBRATION_BAD_RESULT     , "In Progress" } };

        rs2_calibration_status calibration_status = calib_status;

        // We don't know if AC really started working so we add a timeout for not blocking the viewer forever.
        if (!process_started)
        {
            if (cah_process_timeout.has_expired())
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
        cah_state = model_state_type::TRIGGER_MODAL;
        calib_status = RS2_CALIBRATION_RETRY;
        process_started = false;
        registered_to_callback = false;
    }
    return keep_showing;
}

bool cah_model::prompt_reset_camera_accuracy_health(device_model & dev_model, ux_window& window, const std::string& error_message)
{
    bool keep_showing = true;
    bool yes_was_chosen = false;

    std::string message_text("This will reset the camera settings to their factory-calibrated state.\nYou will lose any improvements made with Camera Accuracy Health.\n\n Are you sure?");
    if (yes_no_dialog("Camera Accuracy Health Reset", message_text, yes_was_chosen, window, error_message))
    {
        if (yes_was_chosen)
        {
            auto itr = std::find_if(dev_model.subdevices.begin(), dev_model.subdevices.end(), [](std::shared_ptr<subdevice_model> sub)
            {
                if (sub->s->as<depth_sensor>())
                    return true;
                return false;
            });


            if (itr != dev_model.subdevices.end())
            {
                auto sd = *itr;
                // Trigger CAH process
                sd->s->set_option(RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH, 1.0f);
            }
        }
        keep_showing = false;
    }

    return keep_showing;
}
