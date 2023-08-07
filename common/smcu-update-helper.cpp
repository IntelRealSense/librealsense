// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "smcu-update-helper.h"
#include "model-views.h"
#include "viewer.h"
#include "ux-window.h"

#include "os.h"

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>


constexpr const char* recommended_fw_url = "https://dev.intelrealsense.com/docs/firmware-updates";

namespace rs2
{
    enum smcu_update_ui_state
    {
        RS2_SMCU_STATE_INITIAL_PROMPT = 0,
        RS2_SMCU_STATE_IN_PROGRESS = 1,
        RS2_SMCU_STATE_COMPLETE = 2,
        RS2_SMCU_STATE_FAILED = 3,
    };

    void safety_mcu_update_manager::process_flow(std::function<void()> cleanup, invoker invoke)
    {
        std::string serial = "";
        if (_dev.supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
            serial = _dev.get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);
        else
            serial = _dev.query_sensors().front().get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);


        // Clear FW update related notification to avoid dismissing the notification on ~device_model()
        // We want the notification alive during the whole process.
        _model.related_notifications.erase(
            std::remove_if(_model.related_notifications.begin(),
                _model.related_notifications.end(),
                [](std::shared_ptr< notification_model > n) {
                    return n->is< fw_update_notification_model >();
                }), end(_model.related_notifications));

        for (auto&& n : _model.related_notifications)
        {
            if (n->is< fw_update_notification_model >()
                || n->is< sw_recommended_update_alert_model >())
                n->dismiss(false);
        }

        _progress = 5;

        int next_progress = 10;

        update_device dfu{};

        if (auto upd = _dev.as<updatable>())
        {
            next_progress = 40;

            log("Requesting to switch to recovery mode");

                // in order to update device to DFU state, it will be disconnected then switches to DFU state
                // if querying devices is called while device still switching to DFU state, an exception will be thrown
                // to prevent that, a blocking is added to make sure device is updated before continue to next step of querying device
                upd.enter_update_safety_mcu_state();

                // TODO: HKR DFU issue - remove d500_device usage when HKR will support FIRMWARE_UPDATE_ID
                if (!check_for([this, serial, &dfu]() {
                    auto devs = _ctx.query_devices();

                    for (uint32_t j = 0; j < devs.size(); j++)
                    {
                        try
                        {
                            auto d = devs[j];
                            if (d.is<update_device>())
                            {
                                if (d.supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
                                {
                                    dfu = d;
                                    return true;
                                }
                            }
                        }
                        catch (std::exception& e) {
                            if (auto not_model_protected = get_protected_notification_model())
                            {
                                not_model_protected->output.add_log(RS2_LOG_SEVERITY_WARN,
                                    __FILE__,
                                    __LINE__,
                                    rsutils::string::from() << "Exception caught in FW Update process-flow: " << e.what() << "; Retrying...");
                            }
                        }
                        catch (...) {}
                    }

                    return false;
                    }, cleanup, std::chrono::seconds(60)))
                {
                    fail("Recovery device did not connect in time!");
                    return;
                }
        }
        else
        {
            dfu = _dev.as<update_device>();
        }

        if (dfu)
        {
            _progress = float(next_progress);

            log("Recovery device connected, starting update");

            dfu.update(_smcu, [&](const float progress)
                {
                    _progress = ((ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress);
                });

            // D400 devices takes 3 seconds after image transition until DFU process finish.
            // D500 only starts the process after the image is transferred and it takes much time..
            log("Safety MCU Download completed, await DFU transition event\n"
                "Internal write is in progress\n"
                "Please DO NOT DISCONNECT the camera (might take a few minutes)");
            std::this_thread::sleep_for(std::chrono::seconds(60));

            log("Safety MCU Update completed, waiting for device to reconnect");
        }
        else
        {
            auto upd = _dev.as<updatable>();
            upd.update_unsigned(_smcu, [&](const float progress)
                {
                    _progress = (ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress;
                });

            log("Safety MCU Update completed, waiting for device to reconnect");
        }

        if (!check_for([this, serial]() {
            auto devs = _ctx.query_devices();

            for (uint32_t j = 0; j < devs.size(); j++)
            {
                try
                {
                    auto d = devs[j];

                    if (d.query_sensors().size() && d.query_sensors().front().supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
                    {
                        auto s = d.query_sensors().front().get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);
                        if (s == serial)
                        {
                            log("Discovered connection of the original device");
                            return true;
                        }
                    }
                }
                catch (...) {}
            }

            return false;
            }, cleanup, std::chrono::seconds(120))) // TODO: HKR DFU issue - increased timeout from 60 to 120 seconds for HKR to complete FW write to flash
        {
            fail("Original device did not reconnect in time!");
            return;
        }

        log("Device reconnected succesfully!");

        _progress = 100;

        _done = true;
    }

    bool safety_mcu_update_manager::check_for(std::function<bool()> action, std::function<void()> cleanup, std::chrono::system_clock::duration delta)
    {
        using namespace std;
        using namespace std::chrono;

        auto start = system_clock::now();
        auto now = system_clock::now();
        do
        {
            try
            {
                if (action()) return true;
            }
            catch (const error& e)
            {
                fail(error_to_string(e));
                cleanup();
                return false;
            }
            catch (const std::exception& ex)
            {
                fail(ex.what());
                cleanup();
                return false;
            }
            catch (...)
            {
                fail("Unknown error during update.\nPlease reconnect the camera to exit recovery mode");
                cleanup();
                return false;
            }

            now = system_clock::now();
            this_thread::sleep_for(milliseconds(100));
        } while (now - start < delta);
        return false;
    }
    
    safety_mcu_update_notification_model::safety_mcu_update_notification_model(std::string name, std::shared_ptr<safety_mcu_update_manager> manager, bool exp)
        : process_notification_model(manager)
    {
        enable_expand = false;
        expanded = exp;
        if (expanded) visible = false;

        message = name;
        this->severity = RS2_LOG_SEVERITY_INFO;
        this->category = RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED;
        pinned = true;
        forced = true;
    }

    void safety_mcu_update_notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        using namespace std;
        using namespace chrono;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
            { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != RS2_SMCU_STATE_COMPLETE)
        {
            ImGui::Text("Safety MCU Update Recommended!");

            ImGui::SetCursorScreenPos({ float(x + 5), float(y + 27) });

            draw_text(get_title().c_str(), x, y, height - 50);

            ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 67) });

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1.f - t));

            if (update_state == RS2_SMCU_STATE_INITIAL_PROMPT)
                ImGui::Text("Safety MCU updates offer critical bug fixes and\nunlock new camera capabilities.");
            else
                ImGui::Text("Safety MCU update is underway...\nPlease do not disconnect the device");

            ImGui::PopStyleColor();
        }
        else
        {
            //ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_blue, 1.f - t));
            ImGui::Text("Update Completed");
            //ImGui::PopStyleColor();

            ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });
            ImGui::PushFont(win.get_large_font());
            std::string txt = rsutils::string::from() << textual_icons::throphy;
            ImGui::Text("%s", txt.c_str());
            ImGui::PopFont();

            ImGui::SetCursorScreenPos({ float(x + 40), float(y + 35) });
            ImGui::Text("Camera Safety MCU Updated Successfully");
        }

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        const auto bar_width = width - 115;

        if (update_manager)
        {
            if (update_state == RS2_SMCU_STATE_INITIAL_PROMPT)
            {
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string button_name = rsutils::string::from() << "Install" << "##fwupdate" << index;

                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }) || update_manager->started())
                {
                    // stopping stream before starting fw update
                    auto fw_update_manager = dynamic_cast<firmware_update_manager*>(update_manager.get());
                    std::for_each(fw_update_manager->get_device_model().subdevices.begin(),
                        fw_update_manager->get_device_model().subdevices.end(),
                        [&](const std::shared_ptr<subdevice_model>& sm)
                        {
                            if (sm->streaming)
                            {
                                try
                                {
                                    sm->stop(fw_update_manager->get_protected_notification_model());
                                }
                                catch (...)
                                {
                                    // avoiding exception that can be sent by stop method
                                    // this could happen if the sensor is not streaming and the stop method is called - for example
                                }
                            }
                        });

                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };

                    if (!update_manager->started())
                        update_manager->start(invoke);

                    update_state = RS2_SMCU_STATE_IN_PROGRESS;
                    enable_dismiss = false;
                    _progress_bar.last_progress_time = system_clock::now();
                }
                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "New Safety MCU will be flashed to the device");
                }
            }
            else if (update_state == RS2_SMCU_STATE_IN_PROGRESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_SMCU_STATE_COMPLETE;
                    pinned = false;
                    _progress_bar.last_progress_time = last_interacted = system_clock::now();
                }

                if (!expanded)
                {
                    if (update_manager->failed())
                    {
                        update_manager->check_error(error_message);
                        update_state = RS2_SMCU_STATE_FAILED;
                        pinned = false;
                        dismiss(false);
                    }

                    draw_progress_bar(win, bar_width);

                    ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

                    string id = rsutils::string::from() << "Expand" << "##" << index;
                    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                    if (ImGui::Button(id.c_str(), { 100, 20 }))
                    {
                        expanded = true;
                    }

                    ImGui::PopStyleColor();
                }
            }
        }
        else
        {
            std::string button_name = rsutils::string::from() << "Learn More..." << "##" << index;

            if (ImGui::Button(button_name.c_str(), { float(bar_width), 20 }))
            {
                open_url(recommended_fw_url);
            }
            if (ImGui::IsItemHovered())
            {
                win.link_hovered();
                ImGui::SetTooltip("%s", "Internet connection required");
            }
        }
    }

    void safety_mcu_update_notification_model::draw_expanded(ux_window& win, std::string& error_message)
    {
        if (update_manager->started() && update_state == RS2_SMCU_STATE_INITIAL_PROMPT)
            update_state = RS2_SMCU_STATE_IN_PROGRESS;

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(500, 100));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        std::string title = "Safety MCU Update";
        if (update_manager->failed()) title += " Failed";

        ImGui::OpenPopup(title.c_str());
        if (ImGui::BeginPopupModal(title.c_str(), nullptr, flags))
        {
            ImGui::SetCursorPosX(200);
            std::string progress_str = rsutils::string::from() << "Progress: " << update_manager->get_progress() << "%";
            ImGui::Text("%s", progress_str.c_str());

            ImGui::SetCursorPosX(5);

            draw_progress_bar(win, 490);

            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            auto s = update_manager->get_log();
            ImGui::InputTextMultiline("##fw_update_log", const_cast<char*>(s.c_str()),
                s.size() + 1, { 490,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();

            ImGui::SetCursorPosX(190);
            if (visible || update_manager->done() || update_manager->failed())
            {
                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    if (update_manager->done() || update_manager->failed())
                    {
                        update_state = RS2_SMCU_STATE_FAILED;
                        pinned = false;
                        dismiss(false);
                    }
                    expanded = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
                ImGui::PushStyleColor(ImGuiCol_Text, transparent);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, transparent);
                ImGui::Button("OK", ImVec2(120, 0));
                ImGui::PopStyleColor(5);
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        error_message = "";
    }

    int safety_mcu_update_notification_model::calc_height()
    {
        if (update_state != RS2_SMCU_STATE_COMPLETE) return 150;
        else return 65;
    }

    void safety_mcu_update_notification_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);

        ImGui::PopStyleColor(1);

        ImVec4 c;

        if (update_state == RS2_SMCU_STATE_COMPLETE)
        {
            c = alpha(saturate(light_blue, 0.7f), 1 - t);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
        }
        else
        {
            c = alpha(sensor_bg, 1 - t);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
        }
    }
}
