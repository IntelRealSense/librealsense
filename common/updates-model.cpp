// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <glad/glad.h>
#include "updates-model.h"
#include "model-views.h"
#include "os.h"
#include "res/l515-icon.h"
#include <stb_image.h>
#include "sw-update/http-downloader.h"

using namespace rs2;
using namespace sw_update;
using namespace http;

void updates_model::draw(viewer_model& viewer, ux_window& window, std::string& error_message)
{
    // Protect resources
    static std::vector<update_profile_model> updates_copy;
    {
        std::lock_guard<std::mutex> lock(_lock);
        updates_copy = _updates;
    }

    // Prepare camera icon
    if (!_icon)
    {
        _icon = std::make_shared<rs2::texture_buffer>();
        int x, y, comp;
        auto data = stbi_load_from_memory(camera_icon_l515_png_data, camera_icon_l515_png_size, &x, &y, &comp, 4);
        _icon->upload_image(x, y, data);
        stbi_image_free(data);

        _progress.last_progress_time = std::chrono::system_clock::now();
    }

    const auto window_name = "Updates Window";
     
    //Main window pop up only if essential updates exists
    if (updates_copy.size())
    {
        ImGui::OpenPopup(window_name);
    }

    position_params positions;

    positions.w = window.width()  * 0.6f;
    positions.h = 600;
    positions.x0 = window.width()  * 0.2f;
    positions.y0 = std::max(window.height() - positions.h, 0.f) / 2;
    ImGui::SetNextWindowPos({ positions.x0, positions.y0 });
    ImGui::SetNextWindowSize({ positions.w, positions.h });

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

    if (ImGui::BeginPopupModal(window_name, nullptr, flags))
    {

        std::string title_message = "SOFTWARE UPDATES";
        auto title_size = ImGui::CalcTextSize(title_message.c_str());
        ImGui::SetCursorPosX(positions.w / 2 - title_size.x / 2);
        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", title_message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        positions.orig_pos = ImGui::GetCursorScreenPos();
        positions.mid_y = (positions.orig_pos.y + positions.y0 + positions.h - 30) / 2;

        ImGui::GetWindowDrawList()->AddRectFilled({ positions.orig_pos.x + 140.f, positions.orig_pos.y },
            { positions.x0 + positions.w - 5, positions.y0 + positions.h - 30 }, ImColor(header_color));
        ImGui::GetWindowDrawList()->AddLine({ positions.orig_pos.x + 145.f, positions.mid_y },
            { positions.x0 + positions.w - 10, positions.mid_y }, ImColor(sensor_bg));

        // ===========================================================================
        // Draw Left Pane
        // ===========================================================================

        for (int i = 0; i < static_cast<int>(updates_copy.size()); i++)
        {
            auto& update = updates_copy[i];

            if (ImGui::GetCursorPosY() + 150 > positions.h) break;

            auto pos = ImGui::GetCursorScreenPos();

            if (i == selected_index)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(pos,
                    { pos.x + 140.f, pos.y + 185.f }, ImColor(header_color));
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);

            ImGui::Image((void*)(intptr_t)(_icon->get_gl_handle()), ImVec2{ 128.f, 114.f });

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 140);
            ImGui::PushStyleColor(ImGuiCol_Border, transparent);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);

            std::string limited_name = update.profile.device_name.substr(0, 40);
            ImGui::Text("%s", limited_name.c_str());
            ImGui::PopStyleColor(7);
            ImGui::PopTextWrapPos();

            auto sn_size = ImGui::CalcTextSize(update.profile.serial_number.c_str());
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 70 - sn_size.x / 2);
            ImGui::Text("%s", update.profile.serial_number.c_str());
        }

        auto& update = updates_copy[selected_index];

        bool sw_update_needed(false), fw_update_needed(false);

        // Verify Device Exists
        if (update.profile.dev_active || _fw_update_state == fw_update_states::started)
        {
            // ===========================================================================
            // Draw Software update Pane
            // ===========================================================================
            sw_update_needed = draw_software_section(window_name, update, positions, window, error_message);

            // ===========================================================================
            // Draw Firmware update Pane
            // ===========================================================================
            fw_update_needed = draw_firmware_section(viewer, window_name, update, positions, window, error_message);

        }
        else 
        {
            ImGui::PushFont(window.get_large_font());
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::SetCursorPos({ positions.orig_pos.x, positions.y0 - 100 });
            ImGui::SetWindowFontScale(1.5);
            ImGui::Text("%s","THE DEVICE HAS BEEN DISCONNECTED,");
            ImGui::SetCursorPos({ positions.orig_pos.x - 100, positions.y0 - 70 });
            ImGui::Text("%s", "PLEASE RECONNECT IT OR CLOSE THE UPDATES WINDOW.");
            ImGui::PopFont();
            ImGui::SetCursorPos({ positions.orig_pos.x + 230, positions.y0 });
            ImGui::SetWindowFontScale(3.);
            ImGui::Text("%s", static_cast<const char *>(textual_icons::lock));
            ImGui::SetWindowFontScale(1.);
            ImGui::PopStyleColor();
            

        }
        // ===========================================================================
        // Draw Lower Pane
        // ===========================================================================

        auto no_update_needed = (!sw_update_needed && !fw_update_needed);

        if (!no_update_needed)
        {
            ImGui::SetCursorPos({ 145, positions.h - 25 });
            if (emphasize_dismiss_text)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_red);
            }

            ImGui::Checkbox("I understand and would like to proceed anyway without updating", &ignore);

            if (emphasize_dismiss_text)
            {
                ImGui::PopStyleColor();
            }
        }

        ImGui::SetCursorPos({ positions.w - 145, positions.h - 25 });

        auto enabled = ignore || no_update_needed;
        if (enabled)
        {
            if (_fw_update_state != fw_update_states::started)
            {
                if (ImGui::Button("Close", { 120, 20 }))
                {
                    {
                        std::lock_guard<std::mutex> lock(_lock);
                        _updates.clear();
                    }

                    ImGui::CloseCurrentPopup();
                    emphasize_dismiss_text = false;
                    ignore = false;
                    _fw_update_state = fw_update_states::ready;
                    _fw_download_progress = 0;
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                ImGui::Button("Close", { 120, 20 });
                ImGui::PopStyleColor();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);

            if (ImGui::Button("Close", { 120, 20 }))
            {
                emphasize_dismiss_text = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("To close this window you must install all essential update\n"
                    "or agree to the warning of closing without it");

            }
            ImGui::PopStyleColor(2);
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

}

bool updates_model::draw_software_section(const char * window_name, update_profile_model& selected_profile, position_params& pos, ux_window& window, std::string& error_message)
{
    bool essential_sw_update_needed(false);
    bool recommended_sw_update_needed(false);
    {
        // Prepare sorted array of SW updates profiles
        // Assumption - essential updates version <= other policies versions
        std::vector<dev_updates_profile::update_description> software_updates;
        for (auto&& swu : selected_profile.profile.software_versions)
            software_updates.push_back(swu.second);
        std::sort(software_updates.begin(), software_updates.end(), [](dev_updates_profile::update_description& a, dev_updates_profile::update_description& b) {
            return a.ver < b.ver;
        });
        if (static_cast<int>(software_updates.size()) <= selected_software_update_index) selected_software_update_index = 0;

        dev_updates_profile::update_description selected_software_update;
        if (software_updates.size() != 0)
        {
            bool essential_found(false);
            bool recommended_found(false);
            for (auto sw_update : software_updates)
            {
                if (!essential_found)
                {
                    essential_found = essential_found || sw_update.name.find("ESSENTIAL") != std::string::npos;
                    essential_sw_update_needed = essential_sw_update_needed || essential_found && (selected_profile.profile.software_version < sw_update.ver);
                }

                if (!recommended_found)
                {
                    recommended_found = recommended_found || sw_update.name.find("RECOMMENDED") != std::string::npos;
                    recommended_sw_update_needed = recommended_sw_update_needed || recommended_found && (selected_profile.profile.software_version < sw_update.ver);
                }
            }

            // If essential update found on DB but not needed - Remove it
            if (essential_found && !essential_sw_update_needed)
            {
                auto it = std::find_if(software_updates.begin(), software_updates.end(), [&](dev_updates_profile::update_description& u) {
                    return (u.name.find("ESSENTIAL") != std::string::npos);
                });
                if (it != software_updates.end())
                    software_updates.erase(it);
            }

            // If recommended update found on DB but not needed - Remove it
            if (recommended_found && !recommended_sw_update_needed)
            {
                auto it = std::find_if(software_updates.begin(), software_updates.end(), [&](dev_updates_profile::update_description& u) {
                    return (u.name.find("RECOMMENDED") != std::string::npos);
                });
                if (it != software_updates.end())
                    software_updates.erase(it);
            }
            
            if (essential_sw_update_needed || recommended_sw_update_needed)
            {
                selected_software_update = software_updates[selected_software_update_index];
            }
        }

        ImVec2 sw_text_pos(pos.orig_pos.x + 150, pos.orig_pos.y + 10);
        ImGui::SetCursorScreenPos(sw_text_pos);

        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("LibRealSense SDK: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (!essential_sw_update_needed || software_updates.size() == 0)
        {
            if (recommended_sw_update_needed)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::Text("Recommended update available!");
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::Text("%s", static_cast<const char *>(textual_icons::check_square_o));
                ImGui::SameLine();
                ImGui::Text("%s", "Up to date.");
            }
            ImGui::PopStyleColor();
        }
        else if (essential_sw_update_needed)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, yellowish);
            ImGui::Text("Essential update is available. Please install!");
            ImGui::PopStyleColor();
        }


        ImGui::PopFont();

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        sw_text_pos.y += 30;
        ImGui::SetCursorScreenPos(sw_text_pos);
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", "Content:"); ImGui::SameLine();
        ImGui::PopStyleColor();
        ImGui::Text("%s", "Intel RealSense SDK 2.0, Intel RealSense Viewer and Depth Quality Tool");

        sw_text_pos.y += 20;
        ImGui::SetCursorScreenPos(sw_text_pos);
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", "Purpose:"); ImGui::SameLine();
        ImGui::PopStyleColor();
        ImGui::Text("%s", "Enhancements for stream alignment, texture mapping and camera accuracy health algorithms");

        sw_text_pos.y += 30;
        ImGui::SetCursorScreenPos(sw_text_pos);
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", "Current SW version:");
        ImGui::SameLine();
        ImGui::PopStyleColor();
        auto current_sw_ver_str = std::string(selected_profile.profile.software_version);
        ImGui::Text("%s", current_sw_ver_str.c_str());

        if (essential_sw_update_needed)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, yellowish);
            ImGui::Text("%s", " (Your version is older than the minimum version required for the proper functioning of your device)");
            ImGui::PopStyleColor();
        }

        if (selected_software_update.ver != versions_db_manager::version(0))
        {
            sw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(sw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);

            ImGui::Text("%s", (software_updates.size() >= 2) ?
                "Versions available:" :
                "Version to download:");
            ImGui::SameLine();
            ImGui::PopStyleColor();
            // 3 box for multiple versions
            if (software_updates.size() >= 2)
            {
                std::vector<const char*> swu_labels;
                for (auto&& swu : software_updates)
                {
                    swu_labels.push_back(swu.name.c_str());
                }
                ImGui::PushStyleColor(ImGuiCol_BorderShadow, dark_grey);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, sensor_bg);

                std::string combo_id = "##Software Update Version";
                ImGui::PushItemWidth(200);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
                ImGui::Combo(combo_id.c_str(), &selected_software_update_index, swu_labels.data(), static_cast<int>(swu_labels.size()));
                ImGui::PopItemWidth();
                ImGui::PopStyleColor(2);
                ImGui::SetWindowFontScale(1.);
            }
            else
            {   // Single version
                ImGui::Text("%s", std::string(selected_software_update.ver).c_str());
            }
        }

        if (selected_software_update.release_page != "")
        {
            sw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(sw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("%s", "Release Link:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::Text("%s", selected_software_update.release_page.c_str());

            ImGui::SameLine();
            auto underline_start = ImVec2(ImGui::GetCursorScreenPos().x - (ImGui::CalcTextSize(selected_software_update.release_page.c_str()).x + 8), ImGui::GetCursorScreenPos().y + ImGui::GetFontSize());
            auto underline_end = ImVec2(ImGui::GetCursorScreenPos().x - 8, ImGui::GetCursorScreenPos().y + ImGui::GetFontSize());
            ImGui::GetWindowDrawList()->AddLine(underline_start, underline_end, ImColor(light_grey));

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                window.link_hovered();
            if (ImGui::IsItemClicked())
            {
                try
                {
                    open_url(selected_software_update.release_page.c_str());
                }
                catch (...)
                {
                    LOG_ERROR("Error opening URL: " + selected_software_update.release_page);
                }
            }
        }


        if (selected_software_update.description != "")
        {
            sw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(sw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);

            ImGui::Text("%s", "Description:");
            ImGui::PopStyleColor();

            ImGui::PushTextWrapPos(pos.w - 150);
            ImGui::PushStyleColor(ImGuiCol_Border, transparent);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            auto msg = selected_software_update.description.c_str();
            sw_text_pos.x -= 4;
            sw_text_pos.y += 15;
            ImGui::SetCursorScreenPos(sw_text_pos);
            ImGui::InputTextMultiline("##Software Update Description", const_cast<char*>(msg),
                strlen(msg) + 1, ImVec2(ImGui::GetContentRegionAvailWidth() - 150, pos.mid_y - (pos.orig_pos.y + 160) - 40),
                ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(7);
            ImGui::PopTextWrapPos();
        }

        if (selected_software_update.ver != versions_db_manager::version(0))
        {
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + pos.w - 150, pos.mid_y - 45 });
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, dark_grey);
            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);

            if (ImGui::Button("Download", { 120, 40 }))
            {
                try
                {
                    open_url(selected_software_update.download_link.c_str());
                }
                catch (...)
                {
                    LOG_ERROR("Error opening URL: " + selected_software_update.download_link);
                }
            }

            if (ImGui::IsItemHovered())
            {
                std::string tooltip = "This will redirect you to download the selected software from:\n" + selected_software_update.download_link;
                ImGui::SetTooltip("%s", tooltip.c_str());
                window.link_hovered();
            }

            ImGui::PopStyleColor(3);
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + 150,  pos.mid_y - 25 });
            ImGui::Text("%s", "Visit the release page before download to identify the most suitable package.");
        }

        ImGui::PopStyleColor();
    }
    return essential_sw_update_needed;
}
bool updates_model::draw_firmware_section(viewer_model& viewer, const char * window_name, update_profile_model& selected_profile, position_params& pos, ux_window& window, std::string& error_message)
{
    bool essential_fw_update_needed(false);
    bool recommended_fw_update_needed(false);

    // Prepare sorted array of FW updates profiles
    // Assumption - essential updates version <= other policies versions
    std::vector<dev_updates_profile::update_description> firmware_updates;
    for (auto&& swu : selected_profile.profile.firmware_versions)
        firmware_updates.push_back(swu.second);
    std::sort(firmware_updates.begin(), firmware_updates.end(), [](dev_updates_profile::update_description& a, dev_updates_profile::update_description& b) {
        return a.ver < b.ver;
    });
    if (static_cast<int>(firmware_updates.size()) <= selected_firmware_update_index) selected_firmware_update_index = 0;

    dev_updates_profile::update_description selected_firmware_update;

    if (firmware_updates.size() != 0)
    {
        bool essential_found(false);
        bool recommended_found(false);

        for (auto fw_update : firmware_updates)
        {
            if (!essential_found)
            {
                essential_found = essential_found || fw_update.name.find("ESSENTIAL") != std::string::npos;
                essential_fw_update_needed = essential_fw_update_needed || essential_found && (selected_profile.profile.firmware_version < fw_update.ver);
            }

            if (!recommended_found)
            {
                recommended_found = recommended_found || fw_update.name.find("RECOMMENDED") != std::string::npos;
                recommended_fw_update_needed = recommended_fw_update_needed || recommended_found && (selected_profile.profile.firmware_version < fw_update.ver);
            }
        }

        // If essential update found on DB but not needed - Remove it
        if (essential_found && !essential_fw_update_needed)
        {
            auto it = std::find_if(firmware_updates.begin(), firmware_updates.end(), [&](dev_updates_profile::update_description& u) {
                return (u.name.find("ESSENTIAL") != std::string::npos);
            });
            if (it != firmware_updates.end())
                firmware_updates.erase(it);
        }

        // If recommended update found on DB but not needed - Remove it
        if (recommended_found && !recommended_fw_update_needed)
        {
            auto it = std::find_if(firmware_updates.begin(), firmware_updates.end(), [&](dev_updates_profile::update_description& u) {
                return (u.name.find("RECOMMENDED") != std::string::npos);
            });
            if (it != firmware_updates.end())
                firmware_updates.erase(it);
        }
        if (essential_fw_update_needed || recommended_fw_update_needed)
        {
            selected_firmware_update = firmware_updates[selected_firmware_update_index];
        }

    }

    ImVec2 fw_text_pos(pos.orig_pos.x + 150, pos.mid_y + 15);
    ImGui::SetCursorScreenPos(fw_text_pos);

    ImGui::PushFont(window.get_large_font());
    ImGui::PushStyleColor(ImGuiCol_Text, white);
    ImGui::Text("FIRMWARE: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (!essential_fw_update_needed || firmware_updates.size() == 0)
    {
        if (recommended_fw_update_needed)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("Recommended update available!");
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::Text("%s", static_cast<const char *>(textual_icons::check_square_o));
            ImGui::SameLine();
            ImGui::Text("%s", "Up to date.");
        }
        ImGui::PopStyleColor();
    }
    else if (essential_fw_update_needed)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, yellowish);
        ImGui::Text("Essential update is available. Please install!");
        ImGui::PopStyleColor();
    }


    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    fw_text_pos.y += 25;
    ImGui::SetCursorScreenPos(fw_text_pos);
    ImGui::PushStyleColor(ImGuiCol_Text, white);
    ImGui::Text("%s", "Content:"); ImGui::SameLine();
    ImGui::PopStyleColor();
    ImGui::Text("%s", "Signed Firmware Image (.bin file)");

    fw_text_pos.y += 50;
    ImGui::SetCursorScreenPos(fw_text_pos);
    ImGui::PushStyleColor(ImGuiCol_Text, white);
    ImGui::Text("%s", "Current FW version:");
    ImGui::SameLine();
    ImGui::PopStyleColor();
    auto current_fw_ver_str = std::string(selected_profile.profile.firmware_version);
    ImGui::Text("%s", current_fw_ver_str.c_str());

    if (_fw_update_state != fw_update_states::completed)
    {
        if (essential_fw_update_needed)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, yellowish);
            ImGui::Text("%s", " (Your version is older than the minimum version required for the proper functioning of your device)");
            ImGui::PopStyleColor();
        }

        if (selected_firmware_update.ver != versions_db_manager::version(0))
        {
            fw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(fw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);

            ImGui::Text("%s", (firmware_updates.size() >= 2) ?
                "Versions available:" :
                "Version to download:");
            ImGui::SameLine();
            ImGui::PopStyleColor();
            // Combo box for multiple versions
            if (firmware_updates.size() >= 2)
            {
                std::vector<const char*> fwu_labels;
                for (auto&& fwu : firmware_updates)
                {
                    fwu_labels.push_back(fwu.name.c_str());
                }

                ImGui::PushStyleColor(ImGuiCol_BorderShadow, dark_grey);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, sensor_bg);

                std::string combo_id = "##Firmware Update Version";
                ImGui::PushItemWidth(200);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
                ImGui::Combo(combo_id.c_str(), &selected_firmware_update_index, fwu_labels.data(), static_cast<int>(fwu_labels.size()));
                ImGui::PopItemWidth();
                ImGui::PopStyleColor(2);
                ImGui::SetWindowFontScale(1.);
            }
            else
            {   // Single version
                ImGui::Text("%s", std::string(selected_firmware_update.ver).c_str());
            }
        }

        if (selected_firmware_update.release_page != "")
        {
            fw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(fw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("%s", "Release Link:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::Text("%s", selected_firmware_update.release_page.c_str());

            ImGui::SameLine();
            auto underline_start = ImVec2(ImGui::GetCursorScreenPos().x - (ImGui::CalcTextSize(selected_firmware_update.release_page.c_str()).x + 8), ImGui::GetCursorScreenPos().y + ImGui::GetFontSize());
            auto underline_end = ImVec2(ImGui::GetCursorScreenPos().x - 8, ImGui::GetCursorScreenPos().y + ImGui::GetFontSize());
            ImGui::GetWindowDrawList()->AddLine(underline_start, underline_end, ImColor(light_grey));


            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                window.link_hovered();
            if (ImGui::IsItemClicked())
            {
                try
                {
                    open_url(selected_firmware_update.release_page.c_str());
                }
                catch (...)
                {
                    LOG_ERROR("Error opening URL: " + selected_firmware_update.release_page);
                }
            }
        }

        if (selected_firmware_update.description != "")
        {
            fw_text_pos.y += 25;
            ImGui::SetCursorScreenPos(fw_text_pos);
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("%s", "Description:");
            ImGui::PopStyleColor();

            ImGui::PushTextWrapPos(pos.w - 150);
            ImGui::PushStyleColor(ImGuiCol_Border, transparent);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            auto msg = selected_firmware_update.description.c_str();
            fw_text_pos.x -= 4;
            fw_text_pos.y += 15;
            ImGui::SetCursorScreenPos(fw_text_pos);
            ImGui::InputTextMultiline("##Firmware Update Description", const_cast<char*>(msg),
                strlen(msg) + 1, ImVec2(ImGui::GetContentRegionAvailWidth() - 150, 75),
                ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(7);
            ImGui::PopTextWrapPos();
        }


        if ((_fw_update_state == fw_update_states::ready) &&
            (essential_fw_update_needed || recommended_fw_update_needed))
        {
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + pos.w - 150, pos.orig_pos.y + pos.h - 115 });
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);

            if (ImGui::Button("Download &\n   Install", ImVec2(120, 40)) || _retry)
            {
                _retry = false;
                auto link = selected_firmware_update.download_link;
                std::thread download_thread([link, this]() {
                    std::vector<uint8_t> vec;
                    http_downloader client;

                    if (!client.download_to_bytes_vector(link, vec,
                        [this](uint64_t dl_current_bytes, uint64_t dl_total_bytes) -> callback_result {
                        _fw_download_progress = static_cast<int>((dl_current_bytes * 100) / dl_total_bytes);
                        return callback_result::CONTINUE_DOWNLOAD;
                    }))
                    {
                        _fw_update_state = fw_update_states::failed_downloading;
                        LOG_ERROR("Error in download firmware version from: " + link);
                    }

                    _fw_image = vec;

                    _fw_download_progress = 100;
                });
                download_thread.detach();

                _fw_update_state = fw_update_states::downloading;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("This will download selected firmware and install it to the device");
                window.link_hovered();
            }
            ImGui::PopStyleColor(2);
        }
        else if (_fw_update_state == fw_update_states::downloading)
        {
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + 150, pos.orig_pos.y + pos.h - 95 });
            _progress.draw(window, static_cast<int>(pos.w) - 170, _fw_download_progress / 3);
            if (_fw_download_progress == 100)
            {
                _fw_update_state = fw_update_states::started;

                _update_manager = std::make_shared<firmware_update_manager>(viewer,
                    *selected_profile.dev_model, selected_profile.profile.dev, selected_profile.ctx, _fw_image, true
                    );
                auto invoke = [](std::function<void()> action) { action(); };
                _update_manager->start(invoke);
            }
        }
        else if (_fw_update_state == fw_update_states::started)
        {
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + 150, pos.orig_pos.y + pos.h - 95 });
            _progress.draw(window, static_cast<int>(pos.w) - 170, static_cast<int>(_update_manager->get_progress() * 0.66 + 33));
            if (_update_manager->done()) {
                _fw_update_state = fw_update_states::completed;
                _fw_image.clear();
            }

            if (_update_manager->failed()) {
                _fw_update_state = fw_update_states::failed_updating;
                _fw_image.clear();
                _fw_download_progress = 0;
            }
            // Verify an error window will not pop up. ImGui cannot handle 2 PopUpModals in parallel.
            if (!error_message.empty())
            {
                LOG_ERROR("error caught during update process, details: " + error_message);
                error_message.clear();
            }

        }
        else if (_fw_update_state == fw_update_states::failed_downloading ||
            _fw_update_state == fw_update_states::failed_updating)
        {
            ImGui::SetCursorScreenPos({ pos.orig_pos.x + 150, pos.orig_pos.y + pos.h - 95 });
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            std::string text = _fw_update_state == fw_update_states::failed_downloading ?
                "Firmware download failed, check connection and press to retry" :
                "Firmware update process failed, press to retry";
            if (ImGui::Button(text.c_str(), ImVec2(pos.w - 170, 25)))
            {
                _fw_update_state = fw_update_states::ready;
                _retry = true;
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopStyleColor();

    return essential_fw_update_needed;
}

