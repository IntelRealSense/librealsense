// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include <viewer.h>
#include "d500-on-chip-calib.h"


namespace rs2
{
    d500_on_chip_calib_manager::d500_on_chip_calib_manager(viewer_model& viewer, std::shared_ptr<subdevice_model> sub,
        device_model& model, device dev)
        : process_manager("D500 On-Chip Calibration"),
        _model(model),
        _dev(dev),
        _sub(sub)
    {
        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) &&
            std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) != "D500")
        {
            throw std::runtime_error("This Calibration Process cannot be processed with this device");
        }
    }

    std::string d500_on_chip_calib_manager::convert_action_to_json_string()
    {
        std::stringstream ss;
        if (action == RS2_CALIB_ACTION_ON_CHIP_CALIB)
        {
            ss << "{\n calib run }";
        }
        else if (action == RS2_CALIB_ACTION_ON_CHIP_CALIB_DRY_RUN)
        {
            ss << "{\n calib dry run }";
        }
        else if (action == RS2_CALIB_ACTION_ON_CHIP_CALIB_ABORT)
        {
            ss << "{\n calib abort }";
        }
        return ss.str();
    }

    void d500_on_chip_calib_manager::process_flow(std::function<void()> cleanup, invoker invoke)
    {
        std::string json = convert_action_to_json_string();

        auto calib_dev = _dev.as<auto_calibrated_device>();
        float health = 0.f;
        int timeout_ms = 120000; // 2 minutes
        auto ans = calib_dev.run_on_chip_calibration(json, &health,
            [&](const float progress) {_progress = progress; }, timeout_ms);

        // in order to grep new calibration from answer, use:
        // auto new_calib = std::vector<uint8_t>(ans.begin() + 3, ans.end());

        if (_progress == 100.0)
        {
            _done = true;
        }
        else
        {
            // exception must have been thrown from run_on_chip_calibration call
            _failed = true;
        }


    }

    bool d500_on_chip_calib_manager::abort()
    {
        auto calib_dev = _dev.as<auto_calibrated_device>();
        float health = 0.f;
        int timeout_ms = 50000; // 50 seconds
        std::string json = convert_action_to_json_string();
        auto ans = calib_dev.run_on_chip_calibration(json, &health,
            [&](const float progress) {}, timeout_ms);

        // returns 1 on success, 0 on failure
        return (ans[0] == 1);
    }

    void d500_on_chip_calib_manager::prepare_for_calibration()
    {
        // set depth preset as default preset, turn projector ON and depth AE ON
        if (_sub->s->supports(RS2_CAMERA_INFO_NAME) && 
            (std::string(_sub->s->get_info(RS2_CAMERA_INFO_NAME)) == "Stereo Module"))
        {
            auto depth_sensor = _sub->s->as <rs2::depth_sensor>();

            // disabling the depth visual preset change for D555 - not needed
            std::string dev_name = _dev.supports( RS2_CAMERA_INFO_NAME ) ? _dev.get_info( RS2_CAMERA_INFO_NAME ) : "";
            if( dev_name.find( "D555" ) == std::string::npos )
            {
                // set depth preset as default preset
                set_option_if_needed<rs2::depth_sensor>(depth_sensor, RS2_OPTION_VISUAL_PRESET, 1);
            }

            // turn projector ON
            set_option_if_needed<rs2::depth_sensor>(depth_sensor, RS2_OPTION_EMITTER_ENABLED, 1);

            // turn depth AE ON
            set_option_if_needed<rs2::depth_sensor>(depth_sensor, RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
        }
    }

    std::string d500_on_chip_calib_manager::get_device_pid() const
    {
        std::string pid_str;
        if (_dev.supports(RS2_CAMERA_INFO_PRODUCT_ID))
            pid_str = _dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
        return pid_str;
    }

    d500_autocalib_notification_model::d500_autocalib_notification_model(std::string name, 
        std::shared_ptr<process_manager> manager, bool exp)
        : process_notification_model(manager)
    {
        enable_expand = false;
        enable_dismiss = true;
        expanded = exp;
        if (expanded) visible = false;

        message = name;
        this->severity = RS2_LOG_SEVERITY_INFO;
        this->category = RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT;

        pinned = true;
    }

    void d500_autocalib_notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        const auto bar_width = width - 115;
        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
            { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != RS2_CALIB_STATE_COMPLETE)
        {
            if (get_manager().action == d500_on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
                ImGui::Text("%s", "On-Chip Calibration");
            else if (get_manager().action == d500_on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB_DRY_RUN)
                ImGui::Text("%s", "Dry Run On-Chip Calibration");

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1.f - t));

            if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS)
            {
                enable_dismiss = false;
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 27) });
                ImGui::Text("%s", "Camera is being calibrated...\n");
                draw_abort(win, x, y);
            }
            else if (update_state == RS2_CALIB_STATE_ABORT)
            {
                get_manager().action = d500_on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB_ABORT;
                auto _this = shared_from_this();
                auto invoke = [_this](std::function<void()> action) {_this->invoke(action); };
                try
                {
                    update_state = RS2_CALIB_STATE_ABORT_CALLED;
                    _has_abort_succeeded = get_manager().abort();
                }
                catch (...)
                {
                    throw std::runtime_error("Abort could not be performed!");
                }
            }
            else if (update_state == RS2_CALIB_STATE_ABORT_CALLED)
            {
                update_ui_after_abort_called(win, x, y);
            }
            else if (update_state == RS2_CALIB_STATE_INIT_CALIB ||
                update_state == RS2_CALIB_STATE_INIT_DRY_RUN)
            {
                calibration_button(win, x, y, bar_width);
            }
            else if (update_state == RS2_CALIB_STATE_FAILED)
            {
                update_ui_on_failure(win, x, y);
            }

            ImGui::PopStyleColor();
        }
        else
        {
            update_ui_on_calibration_complete(win, x, y);
        }

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        if (update_manager)
        {
            if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_CALIB_STATE_COMPLETE;
                    enable_dismiss = true;
                }
                else if (update_manager->failed())
                {
                    update_state = RS2_CALIB_STATE_FAILED;
                    enable_dismiss = true;
                }

                if (!expanded)
                {
                    if (update_manager->failed())
                    {
                        update_manager->check_error(_error_message);
                        update_state = RS2_CALIB_STATE_FAILED;
                        enable_dismiss = true;
                    }

                    draw_progress_bar(win, bar_width);
                    ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });
                    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                    ImGui::PopStyleColor();
                }
            }
        }
    }

    int d500_autocalib_notification_model::calc_height()
    {
        // adjusting the height of the notification window
        if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS ||
            update_state == RS2_CALIB_STATE_COMPLETE ||
            update_state == RS2_CALIB_STATE_ABORT_CALLED ||
            update_state == RS2_CALIB_STATE_FAILED)
            return 90;
        return 60;
    }


    void d500_autocalib_notification_model::calibration_button(ux_window& win, int x, int y, int bar_width)
    {
        using namespace std;
        using namespace chrono;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - ImGui::GetTextLineHeightWithSpacing() - 31) });

        auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
        ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

        std::string activation_cal_str = "Calibrate";
        if (update_state == RS2_CALIB_STATE_INIT_DRY_RUN)
            activation_cal_str = "Calibrate Dry Run";

        std::string calibrate_button_name = rsutils::string::from() << activation_cal_str << "##self" << index;

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 28) });
        if (ImGui::Button(calibrate_button_name.c_str(), { float(bar_width), 20.f }))
        {
            get_manager().reset();
            if (update_state == RS2_CALIB_STATE_INIT_DRY_RUN)
            {
                get_manager().action = d500_on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB_DRY_RUN;
            }

            get_manager().prepare_for_calibration();

            auto _this = shared_from_this();
            auto invoke = [_this](std::function<void()> action) {_this->invoke(action); };
            get_manager().start(invoke);
            update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
            enable_dismiss = false;
        }
        ImGui::PopStyleColor(2);
    }

    void d500_autocalib_notification_model::draw_abort(ux_window& win, int x, int y)
    {
        ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

        std::string id = rsutils::string::from() << "Abort" << "##" << index;


        ImGui::SetNextWindowPos({ float(x + width - 125), float(y + height - 25) });
        ImGui::SetNextWindowSize({ 120, 70 });

        if (ImGui::Button(id.c_str(), { 100, 20 }))
        {
            update_state = RS2_CALIB_STATE_ABORT;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Abort Calibration Process");
        }
    }

    void d500_autocalib_notification_model::update_ui_after_abort_called(ux_window& win, int x, int y)
    {
        ImGui::Text("%s", "Calibration Aborting");
        ImGui::SetCursorScreenPos({ float(x + 10), float(y + 40) });
        ImGui::PushFont(win.get_large_font());
        std::string txt = rsutils::string::from() << textual_icons::stop;
        ImGui::Text("%s", txt.c_str());
        ImGui::PopFont();

        ImGui::SetCursorScreenPos({ float(x + 40), float(y + 40) });
        if (_has_abort_succeeded)
        {
            ImGui::Text("%s", "Camera Calibration Aborted Successfully");
        }
        else
        {
            ImGui::Text("%s", "Camera Calibration Could not be Aborted");
        }
        enable_dismiss = true;
    }
    
    void d500_autocalib_notification_model::update_ui_on_failure(ux_window& win, int x, int y)
    {
        ImGui::SetCursorScreenPos({ float(x + 50), float(y + 50) });
        ImGui::Text("%s", "Calibration Failed");
        ImGui::SetCursorScreenPos({ float(x + 10), float(y + 50) });
        ImGui::PushFont(win.get_large_font());
        std::string txt = rsutils::string::from() << textual_icons::exclamation_triangle;
        ImGui::Text("%s", txt.c_str());
        ImGui::PopFont();

        ImGui::SetCursorScreenPos({ float(x + 40), float(y + 50) });
        
        enable_dismiss = true;
    }

    void d500_autocalib_notification_model::update_ui_on_calibration_complete(ux_window& win, int x, int y)
    {
        ImGui::Text("%s", "Calibration Complete");

        ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });
        ImGui::PushFont(win.get_large_font());
        std::string txt = rsutils::string::from() << textual_icons::throphy;
        ImGui::Text("%s", txt.c_str());
        ImGui::PopFont();

        ImGui::SetCursorScreenPos({ float(x + 40), float(y + 35) });

        ImGui::Text("%s", "Camera Calibration Applied Successfully");
    }
}
