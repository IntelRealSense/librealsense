// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include <glad/glad.h>

#include "fw-update-helper.h"

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <model-views.h>

#include "os.h"

#ifdef INTERNAL_FW
#include "common/fw/D4XX_FW_Image.h"
#include "common/fw/D4XX_RC_Image.h"
#include "common/fw/SR3XX_FW_Image.h"
#else
#define FW_D4XX_FW_IMAGE_VERSION ""
#define FW_D4XX_RC_IMAGE_VERSION ""
#define FW_SR3XX_FW_IMAGE_VERSION ""
const char* fw_get_D4XX_FW_Image(int) { return NULL; }
const char* fw_get_D4XX_RC_Image(int) { return NULL; }
const char* fw_get_SR3XX_FW_Image(int) { return NULL; }
#endif // INTERNAL_FW

constexpr const char* recommended_fw_url = "https://dev.intelrealsense.com/docs/firmware-releases";

namespace rs2
{
    enum firmware_update_ui_state
    {
        RS2_FWU_STATE_INITIAL_PROMPT = 0,
        RS2_FWU_STATE_IN_PROGRESS = 1,
        RS2_FWU_STATE_COMPLETE = 2,
        RS2_FWU_STATE_FAILED = 3,
    };

    bool is_recommended_fw_available(std::string id)
    {
        auto pl = parse_product_line(id);
        auto fv = get_available_firmware_version(pl);
        return !(fv == "");
    }

    int parse_product_line(std::string id)
    {
        if (id == "D400") return RS2_PRODUCT_LINE_D400;
        else if (id == "SR300") return RS2_PRODUCT_LINE_SR300;
        else return -1;
    }

    std::string get_available_firmware_version(int product_line)
    {
        bool allow_rc_firmware = config_file::instance().get_or_default(configurations::update::allow_rc_firmware, false);

        if (product_line == RS2_PRODUCT_LINE_D400 && allow_rc_firmware) return FW_D4XX_RC_IMAGE_VERSION;
        else if (product_line == RS2_PRODUCT_LINE_D400) return FW_D4XX_FW_IMAGE_VERSION;
        //else if (product_line == RS2_PRODUCT_LINE_SR300) return FW_SR3XX_FW_IMAGE_VERSION;
        else return "";
    }

    std::map<int, std::vector<uint8_t>> create_default_fw_table()
    {
        bool allow_rc_firmware = config_file::instance().get_or_default(configurations::update::allow_rc_firmware, false);

        std::map<int, std::vector<uint8_t>> rv;

        if (strlen(FW_D4XX_FW_IMAGE_VERSION) && !allow_rc_firmware)
        {
            int size = 0;
            auto hex = fw_get_D4XX_FW_Image(size);
            auto vec = std::vector<uint8_t>(hex, hex + size);
            rv[RS2_PRODUCT_LINE_D400] = vec;
        }

        if (strlen(FW_D4XX_RC_IMAGE_VERSION) && allow_rc_firmware)
        {
            int size = 0;
            auto hex = fw_get_D4XX_RC_Image(size);
            auto vec = std::vector<uint8_t>(hex, hex + size);
            rv[RS2_PRODUCT_LINE_D400] = vec;
        }

        if (strlen(FW_SR3XX_FW_IMAGE_VERSION))
        {
            int size = 0;
            auto hex = fw_get_SR3XX_FW_Image(size);
            auto vec = std::vector<uint8_t>(hex, hex + size);
            rv[RS2_PRODUCT_LINE_SR300] = vec;
        }

        return rv;
    }

    std::vector<int> parse_fw_version(const std::string& fw)
    {
        std::vector<int> rv;
        size_t pos = 0;
        std::string delimiter = ".";
        auto str = fw + delimiter;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            auto s = str.substr(0, pos);
            int val = std::stoi(s);
            rv.push_back(val);
            str.erase(0, pos + delimiter.length());
        }
        return rv;
    }

    bool is_upgradeable(const std::string& curr, const std::string& available)
    {
        if (curr == "" || available == "") return false;

        size_t fw_string_size = 4;
        auto c = parse_fw_version(curr);
        auto a = parse_fw_version(available);
        if (a.size() != fw_string_size || c.size() != fw_string_size)
            return false;

        for (int i = 0; i < fw_string_size; i++) {
            if (c[i] > a[i])
                return false;
            if (c[i] < a[i])
                return true;
        }
        return false; //equle
    }

    bool firmware_update_manager::check_for(
        std::function<bool()> action, std::function<void()> cleanup,
        std::chrono::system_clock::duration delta)
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

    void firmware_update_manager::process_flow(
        std::function<void()> cleanup,
        invoker invoke)
    {
        std::string serial = "";
        if (_dev.supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
            serial = _dev.get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);
        else
            serial = _dev.query_sensors().front().get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);

        _model.related_notifications.clear();

        _progress = 5;

        int next_progress = 10;

        update_device dfu{};

        if (auto upd = _dev.as<updatable>())
        {
            log("Backing-up camera flash memory");

            auto flash = upd.create_flash_backup([&](const float progress)
            {
                _progress = ((ceil(progress * 5) / 5) * (30 - next_progress)) + next_progress;
            });

            auto temp = get_folder_path(special_folder::app_data);
            temp += serial + "." + get_timestamped_file_name() + ".bin";

            {
                std::ofstream file(temp.c_str(), std::ios::binary);
                file.write((const char*)flash.data(), flash.size());
            }

            std::string log_line = "Backup completed and saved as '";
            log_line += temp + "'";
            log(log_line);

            next_progress = 40;

            if (_is_signed)
            {
                log("Requesting to switch to recovery mode");
                upd.enter_update_state();

                if (!check_for([this, serial, &dfu]() {
                    auto devs = _ctx.query_devices();

                    for (int j = 0; j < devs.size(); j++)
                    {
                        try
                        {
                            auto d = devs[j];
                            if (d.is<update_device>())
                            {
                                if (d.supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
                                {
                                    if (serial == d.get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
                                    {
                                        dfu = d;
                                        return true;
                                    }
                                }
                            }
                        }
                        catch (std::exception &e) {
                            std::stringstream s;
                            s << "Exception caught in FW Update process-flow: " << e.what();
                            log(s.str().c_str());
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
        }
        else
        {
            dfu = _dev.as<update_device>();
        }

        if (dfu)
        {
            _progress = next_progress;

            log("Recovery device connected, starting update");

            dfu.update(_fw, [&](const float progress)
            {
                _progress = (ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress;
            });

            log("Firmware Download completed, await DFU transition event");
            std::this_thread::sleep_for(std::chrono::seconds(3));
            log("Firmware Update completed, waiting for device to reconnect");
        }
        else
        {
            auto upd = _dev.as<updatable>();
            upd.update_unsigned(_fw, [&](const float progress)
            {
                _progress = (ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress;
            });
            log("Firmware Update completed, waiting for device to reconnect");
        }

        if (!check_for([this, serial, &dfu]() {
            auto devs = _ctx.query_devices();

            for (int j = 0; j < devs.size(); j++)
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
        }, cleanup, std::chrono::seconds(60)))
        {
            fail("Original device did not reconnect in time!");
            return;
        }

        log("Device reconnected succesfully!");

        _progress = 100;

        _done = true;
    }

    void fw_update_notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        using namespace std;
        using namespace chrono;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
        { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != RS2_FWU_STATE_COMPLETE)
        {
            ImGui::Text("Firmware Update Recommended!");

            ImGui::SetCursorScreenPos({ float(x + 5), float(y + 27) });

            draw_text(get_title().c_str(), x, y, height - 50);

            ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 67) });

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1. - t));

            if (update_state == RS2_FWU_STATE_INITIAL_PROMPT)
                ImGui::Text("Firmware updates offer critical bug fixes and\nunlock new camera capabilities.");
            else
                ImGui::Text("Firmware update is underway...\nPlease do not disconnect the device");

            ImGui::PopStyleColor();
        }
        else
        {
            //ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_blue, 1.f - t));
            ImGui::Text("Update Completed");
            //ImGui::PopStyleColor();

            ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });
            ImGui::PushFont(win.get_large_font());
            std::string txt = to_string() << textual_icons::throphy;
            ImGui::Text("%s", txt.c_str());
            ImGui::PopFont();

            ImGui::SetCursorScreenPos({ float(x + 40), float(y + 35) });
            ImGui::Text("Camera Firmware Updated Successfully");
        }

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        const auto bar_width = width - 115;

        if (update_manager)
        {
            if (update_state == RS2_FWU_STATE_INITIAL_PROMPT)
            {
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string button_name = to_string() << "Install" << "##fwupdate" << index;

                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }) || update_manager->started())
                {
                    if (!update_manager->started()) update_manager->start(shared_from_this());

                    update_state = RS2_FWU_STATE_IN_PROGRESS;
                    enable_dismiss = false;
                    last_progress_time = system_clock::now();
                }
                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "New firmware will be flashed to the device");
                }
            }
            else if (update_state == RS2_FWU_STATE_IN_PROGRESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_FWU_STATE_COMPLETE;
                    pinned = false;
                    last_progress_time = last_interacted = system_clock::now();
                }

                if (!expanded)
                {
                    if (update_manager->failed())
                    {
                        update_manager->check_error(error_message);
                        update_state = RS2_FWU_STATE_FAILED;
                        pinned = false;
                        dismiss(false);
                    }

                    draw_progress_bar(win, bar_width);

                    ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

                    string id = to_string() << "Expand" << "##" << index;
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
            std::string button_name = to_string() << "Learn More..." << "##" << index;

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

    void fw_update_notification_model::draw_expanded(ux_window& win, std::string& error_message)
    {
        if (update_manager->started() && update_state == RS2_FWU_STATE_INITIAL_PROMPT)
            update_state = RS2_FWU_STATE_IN_PROGRESS;

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

        std::string title = "Firmware Update";
        if (update_manager->failed()) title += " Failed";

        ImGui::OpenPopup(title.c_str());
        if (ImGui::BeginPopupModal(title.c_str(), nullptr, flags))
        {
            ImGui::SetCursorPosX(200);
            std::string progress_str = to_string() << "Progress: " << update_manager->get_progress() << "%";
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
                        update_state = RS2_FWU_STATE_FAILED;
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

    int fw_update_notification_model::calc_height()
    {
        if (update_state != RS2_FWU_STATE_COMPLETE) return 150;
        else return 65;
    }

    void fw_update_notification_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);

        ImGui::PopStyleColor(1);

        ImVec4 c;

        if (update_state == RS2_FWU_STATE_COMPLETE)
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

    fw_update_notification_model::fw_update_notification_model(std::string name,
        std::shared_ptr<firmware_update_manager> manager, bool exp)
        : process_notification_model(manager)
    {
        enable_expand = false;
        expanded = exp;
        if (expanded) visible = false;

        message = name;
        this->severity = RS2_LOG_SEVERITY_INFO;
        this->category = RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED;

        pinned = true;
    }
}
