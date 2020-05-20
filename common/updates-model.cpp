#include <glad/glad.h>
#include "updates-model.h"
#include "model-views.h"
#include "os.h"
#include "res/l515-icon.h"
#include <stb_image.h>
#include "auto-updater/http-downloader.h"

using namespace rs2;

void updates_model::draw(ux_window& window, std::string& error_message)
{
    std::vector<update_profile> updates_copy;
    {
        std::lock_guard<std::mutex> lock(_lock);
        updates_copy = _updates;
    }

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

    if (updates_copy.size() && !ignore)
        ImGui::OpenPopup(window_name);

    float w  = window.width()  * 0.6f;
    float h  = 600;
    float x0 = window.width()  * 0.2f;
    float y0 = std::max(window.height() - h, 0.f) / 2;
    ImGui::SetNextWindowPos({ x0, y0 });
    ImGui::SetNextWindowSize({ w, h });

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

    if (ImGui::BeginPopupModal(window_name, nullptr, flags))
    {
        std::string title_message = "ESSENTIAL UPDATE";
        auto title_size = ImGui::CalcTextSize(title_message.c_str());
        ImGui::SetCursorPosX(w / 2 - title_size.x / 2);
        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text(title_message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5);

        const auto orig_pos = ImGui::GetCursorScreenPos();
        const auto mid_y = (orig_pos.y + y0 + h - 30) / 2;

        ImGui::GetWindowDrawList()->AddRectFilled({ orig_pos.x + 140.f, orig_pos.y },
                    { x0 + w - 5, y0 + h - 30 }, ImColor(header_color));
        ImGui::GetWindowDrawList()->AddLine({ orig_pos.x + 145.f, mid_y },
                    { x0 + w - 10, mid_y }, ImColor(sensor_bg));

        for (int i = 0; i < updates_copy.size(); i++)
        {
            auto& update = updates_copy[i];

            if (ImGui::GetCursorPosY() + 150 > h) break;

            auto pos = ImGui::GetCursorScreenPos();

            if (i == selected_index)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(pos,
                        { pos.x + 140.f, pos.y + 165.f }, ImColor(header_color));
            }
                
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+4);

            ImGui::Image(ImTextureID(_icon->get_gl_handle()), ImVec2{ 128.f, 114.f });

            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+4);
            ImGui::Text("%s", update.device_name.c_str());
            ImGui::PopStyleColor();

            auto sn_size = ImGui::CalcTextSize(update.serial_number.c_str());
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ 70 - sn_size.x / 2);
            ImGui::Text("%s", update.serial_number.c_str());
        }

        auto& update = updates_copy[selected_index];

        std::vector<update_description> software_updates;
        for (auto&& swu : update.software_versions)
            software_updates.push_back(swu.second);
        std::sort(software_updates.begin(), software_updates.end(), [](update_description& a, update_description& b){
            return a.ver < b.ver;
        });
        if (software_updates.size() <= selected_software_update_index) selected_software_update_index = 0;


        std::vector<update_description> firmware_updates;
        for (auto&& swu : update.firmware_versions)
            firmware_updates.push_back(swu.second);
        std::sort(firmware_updates.begin(), firmware_updates.end(), [](update_description& a, update_description& b){
            return a.ver < b.ver;
        });
        if (firmware_updates.size() <= selected_firmware_update_index) selected_firmware_update_index = 0;

        if (software_updates.size() >= 2)
        {
            std::vector<const char*> swu_labels;
            for (auto&& swu : software_updates)
            {
                swu_labels.push_back(swu.name.c_str());
            }

            ImGui::SetCursorScreenPos({ orig_pos.x + w - 185, orig_pos.y + 10 });
            std::string combo_id = "##Software Update Version";
            ImGui::PushItemWidth(170);
            ImGui::Combo(combo_id.c_str(), &selected_software_update_index, swu_labels.data(), swu_labels.size());
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos({ orig_pos.x + w - 240, orig_pos.y + 11 });
            ImGui::Text("%s", "Update:");
        }

        auto& selected_software_update = software_updates[selected_software_update_index];
        auto sw_updated = selected_software_update.ver <= update.software_version;

        if (firmware_updates.size() >= 2 && _fw_update_state == fw_update_states::ready)
        {
            std::vector<const char*> fwu_labels;
            for (auto&& fwu : firmware_updates)
            {
                fwu_labels.push_back(fwu.name.c_str());
            }

            ImGui::SetCursorScreenPos({ orig_pos.x + w - 185, mid_y + 10 });
            std::string combo_id = "##Firmware Update Version";
            ImGui::PushItemWidth(170);
            ImGui::Combo(combo_id.c_str(), &selected_firmware_update_index, fwu_labels.data(), fwu_labels.size());
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos({ orig_pos.x + w - 240, mid_y + 11 });
            ImGui::Text("%s", "Update:");
        }

        auto& selected_firmware_update = firmware_updates[selected_firmware_update_index];
        auto fw_updated = selected_firmware_update.ver <= update.firmware_version;

        ImGui::SetCursorPos({ 145, h - 25 });
        ImGui::Checkbox("I understand and would like to proceed anyway without updating", &ignore);

        ImGui::SetCursorPos({ w - 125, h - 25 });
        auto enabled = ignore || (sw_updated && fw_updated);
        if (enabled)
        {
            if (ImGui::Button("OK", { 120, 20 })) 
            {
                ImGui::CloseCurrentPopup();
                std::lock_guard<std::mutex> lock(_lock);
                _updates.clear();
                ignore = false;
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);
            ImGui::Button("OK", { 120, 20 });
            ImGui::PopStyleColor(5);
        }

        {
            ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 10 });

            ImGui::PushFont(window.get_large_font());
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("SOFTWARE: "); 
            ImGui::PopStyleColor();

            if (sw_updated)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::Text(u8"\uF046 Up-to-Date!");
                ImGui::PopStyleColor();
            }

            auto white_color = sw_updated ? grey : white;
            auto gray_color = sw_updated ? grey : light_grey;
            auto link_color = sw_updated ? grey : light_grey;

            ImGui::PopFont();

            ImGui::PushStyleColor(ImGuiCol_Text, gray_color);

            ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 40 });
            ImGui::PushStyleColor(ImGuiCol_Text, white_color);
            ImGui::Text("%s", "Content:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::Text("%s", "Intel RealSense SDK 2.0, Intel RealSense Viewer and Depth Quality Tool");

            ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 60 });
            ImGui::PushStyleColor(ImGuiCol_Text, white_color);
            ImGui::Text("%s", "Purpose:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::Text("%s", "Responsible for stream alignment, texture mapping and camera accuracy health algorithms");

            if (selected_software_update.ver != version(0))
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 90 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Version:"); ImGui::SameLine();
                ImGui::PopStyleColor();
                ImGui::Text("%s", std::string(selected_software_update.ver).c_str());
            }

            if (selected_software_update.release_page != "")
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 110 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Release Link:"); ImGui::SameLine();
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, link_color);
                ImGui::Text("%s", selected_software_update.release_page.c_str());
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                    window.link_hovered();
                if (ImGui::IsItemClicked())
                {
                    try
                    {
                        open_url(selected_software_update.release_page.c_str());
                    } catch(...)
                    {
                        error_message = "Could not open link";
                        ImGui::CloseCurrentPopup();
                        std::lock_guard<std::mutex> lock(_lock);
                        _updates.clear();
                    }
                }
            }

            if (selected_software_update.description != "")
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + 130 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Description:");
                ImGui::PopStyleColor();

                ImGui::PushTextWrapPos(w - 200);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                auto msg = selected_software_update.description.c_str();
                ImGui::SetCursorScreenPos({ orig_pos.x + 146, orig_pos.y + 145 });
                ImGui::InputTextMultiline("##Software Update Description", const_cast<char*>(msg),
                    strlen(msg) + 1, ImVec2(0,0),
                    ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(6);
                ImGui::PopTextWrapPos();
            }


            ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y - 25 });
            ImGui::Text("%s", "Please follow instructions from the release page (link above) for your combination of platform and OS");

            ImGui::PopStyleColor();
        }

        // ===========================================================================

        {
            ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 15 });

            ImGui::PushFont(window.get_large_font());
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::Text("FIRMWARE: "); 
            ImGui::PopStyleColor();

            if (fw_updated || _fw_update_state == fw_update_states::completed)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::Text(u8"\uF046 Up-to-Date!");
                ImGui::PopStyleColor();
            }

            auto white_color = fw_updated ? grey : white;
            auto gray_color = fw_updated ? grey : light_grey;
            auto link_color = fw_updated ? grey : light_grey;

            ImGui::PopFont();

            ImGui::PushStyleColor(ImGuiCol_Text, gray_color);

            ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 45 });
            ImGui::PushStyleColor(ImGuiCol_Text, white_color);
            ImGui::Text("%s", "Content:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::Text("%s", "Signed Firmware Image (.bin file)");

            ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 65 });
            ImGui::PushStyleColor(ImGuiCol_Text, white_color);
            ImGui::Text("%s", "Purpose:"); ImGui::SameLine();
            ImGui::PopStyleColor();
            ImGui::Text("%s", "Control logic over various hardware subsystems - sensors, projector, IMU, etc...");

            if (selected_firmware_update.ver != version(0))
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 95 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Version:"); ImGui::SameLine();
                ImGui::PopStyleColor();
                ImGui::Text("%s", std::string(selected_firmware_update.ver).c_str());
            }

            if (selected_firmware_update.release_page != "")
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 115 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Release Link:"); ImGui::SameLine();
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, link_color);
                ImGui::Text("%s", selected_firmware_update.release_page.c_str());
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                    window.link_hovered();
                if (ImGui::IsItemClicked())
                {
                    try
                    {
                        open_url(selected_firmware_update.release_page.c_str());
                    } catch(...)
                    {
                        error_message = "Could not open link";
                        ImGui::CloseCurrentPopup();
                        std::lock_guard<std::mutex> lock(_lock);
                        _updates.clear();
                    }
                }
            }

            if (selected_firmware_update.description != "")
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, mid_y + 135 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                ImGui::Text("%s", "Description:");
                ImGui::PopStyleColor();

                ImGui::PushTextWrapPos(w - 200);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                auto msg = selected_firmware_update.description.c_str();
                ImGui::SetCursorScreenPos({ orig_pos.x + 146, mid_y + 150 });
                ImGui::InputTextMultiline("##Firmware Update Description", const_cast<char*>(msg),
                    strlen(msg) + 1, ImVec2(w - 200,80),
                    ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(6);
                ImGui::PopTextWrapPos();
            }


            if (_fw_update_state == fw_update_states::ready)
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + h - 95 });
                ImGui::PushStyleColor(ImGuiCol_Text, white_color);
                if (ImGui::Button("Download & Install", ImVec2(w - 170, 25)))
                {
                    auto link = selected_firmware_update.download_link;
                    std::thread download_thread([link, this](){
                        std::stringstream ss;
                        ss.clear();
                        http_downloader client;

                        client.download_to_stream(link, ss, 
                            [this](uint64_t dl_current_bytes, uint64_t dl_total_bytes, double dl_time) -> bool {
                                _fw_download_progress = (dl_current_bytes * 100) / dl_total_bytes;
                                return 0;
                            });

                        std::vector<uint8_t> vec;
                        uint8_t byte;
                        while(ss >> std::noskipws >> byte) 
                            vec.push_back(byte);
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
                ImGui::PopStyleColor();
            } 
            else if (_fw_update_state == fw_update_states::downloading)
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + h - 95 });
                _progress.draw(window, w - 170, _fw_download_progress / 3);
                if (_fw_download_progress == 100)
                {
                    _fw_update_state = fw_update_states::started;

                    _update_manager = std::make_shared<firmware_update_manager>(
                        *update.dev_model, update.dev, update.ctx, _fw_image, true
                    ); 
                    auto invoke = [](std::function<void()> action) { action(); };
                    _update_manager->start(invoke);
                    // update.dev = rs2::device{};
                    // update.dev_model->dev = rs2::device{};
                }
            } else if (_fw_update_state == fw_update_states::started)
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + h - 95 });
                _progress.draw(window, w - 170, _update_manager->get_progress() * 0.66 + 33);
                if (_update_manager->done()) {
                    _fw_update_state = fw_update_states::completed;
                }

                if (error_message != "") error_message = "";

                if (_update_manager->failed()) {
                    _fw_update_state = fw_update_states::failed;
                    ImGui::CloseCurrentPopup();
                }

            } 
            else if (_fw_update_state == fw_update_states::completed)
            {
                ImGui::SetCursorScreenPos({ orig_pos.x + 150, orig_pos.y + h - 95 });
                //_progress.draw(window, w - 170, 100);
            }

            ImGui::PopStyleColor();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}