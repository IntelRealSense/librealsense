// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <thread>
#include <algorithm>
#include <regex>
#include <cmath>

#include <opengl3.h>

#include "notifications.h"
#include <imgui_internal.h>

#include "os.h"

using namespace std;
using namespace chrono;

namespace rs2
{
    notification_data::notification_data(std::string description,
        rs2_log_severity severity,
        rs2_notification_category category)
        : _description(description),
          _severity(severity),
          _category(category) 
    {
        _timestamp = (double)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    rs2_notification_category notification_data::get_category() const
    {
        return _category;
    }

    std::string notification_data::get_description() const
    {
        return _description;
    }


    double notification_data::get_timestamp() const
    {
        return _timestamp;
    }


    rs2_log_severity notification_data::get_severity() const
    {
        return _severity;
    }

    notification_model::notification_model()
    {
        custom_action = []{};
        last_x = 500000;
        last_y = 200;
        message = "";
        last_moved = std::chrono::system_clock::now();
    }

    notification_model::notification_model(const notification_data& n)
        : notification_model()
    {
        message = n.get_description();
        timestamp = n.get_timestamp();
        severity = n.get_severity();
        created_time = std::chrono::system_clock::now();
        last_interacted = std::chrono::system_clock::now() - std::chrono::milliseconds(500);
        category = n.get_category();
    }

    double notification_model::get_age_in_ms(bool total) const
    {
        auto interacted = duration<double, milli>(last_interacted - created_time).count();
        return duration<double, milli>(system_clock::now() - created_time).count() - (total ? 0.0 : interacted);
    }

    bool notification_model::interacted() const
    {
        return duration<double, milli>(system_clock::now() - last_interacted).count() < 100;
    }


    ImVec4 saturate(const ImVec4& a, float f)
    {
        return{ f * a.x, f * a.y, f * a.z, a.w };
    }

    ImVec4 alpha(const ImVec4& v, float a)
    {
        return{ v.x, v.y, v.z, a };
    }

    // Pops the N colors that were pushed in set_color_scheme
    void notification_model::unset_color_scheme() const
    {
        ImGui::PopStyleColor(6);
    }

    void notification_model::draw_progress_bar(ux_window & win, int bar_width)
    {
        auto progress = update_manager->get_progress();

        auto now = system_clock::now();
        auto ellapsed = duration_cast<milliseconds>(now - last_progress_time).count() / 1000.f;

        auto new_progress = last_progress + ellapsed * progress_speed;
        curr_progress_value = std::min(threshold_progress, new_progress);

        if (last_progress != progress)
        {
            last_progress_time = system_clock::now();

            int delta = progress - last_progress;

            if (ellapsed > 0.f) progress_speed = delta / ellapsed;

            threshold_progress = std::min(100, progress + delta);

            last_progress = progress;
        }

        auto filled_w = (curr_progress_value * (bar_width - 4)) / 100.f;

        auto pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled({ float(pos.x), float(pos.y) },
        { float(pos.x + bar_width), float(pos.y + 20) }, ImColor(black));

        if (curr_progress_value > 0.f)
        {
            for (int i = 20; i >= 0; i -= 2)
            {
                auto a = curr_progress_value / 100.f;
                ImGui::GetWindowDrawList()->AddRectFilled({ float(pos.x + 3 - i), float(pos.y + 3 - i) },
                { float(pos.x + filled_w + i), float(pos.y + 17 + i) },
                    ImColor(alpha(light_blue, sqrt(a) * 0.02f)), i);
            }

            ImGui::GetWindowDrawList()->AddRectFilled({ float(pos.x + 3), float(pos.y + 3) },
                { float(pos.x + filled_w), float(pos.y + 17) }, ImColor(light_blue));

            ImGui::GetWindowDrawList()->AddRectFilledMultiColor({ float(pos.x + 5), float(pos.y + 5) },
                { float(pos.x + filled_w), float(pos.y + 15) },
                ImColor(saturate(light_blue, 1.f)), ImColor(saturate(light_blue, 0.9f)),
                ImColor(saturate(light_blue, 0.8f)), ImColor(saturate(light_blue, 1.f)));

            rs2::rect pbar{ float(pos.x + 3), float(pos.y + 3), bar_width, 17.f };
            auto mouse = win.get_mouse();
            if (pbar.contains(mouse.cursor))
            {
                std::string progress_str = to_string() << progress << "%";
                ImGui::SetTooltip("%s", progress_str.c_str());
            }
        }

        ImGui::SetCursorScreenPos({ float(pos.x), float(pos.y + 25) });
    }

    /* Sets color scheme for notifications, must be used with unset_color_scheme to pop all colors in the end
       Parameter t indicates the transparency of the nofication interface */
    void notification_model::set_color_scheme(float t) const
    {
        ImVec4 c;

        if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            if (update_state == 2)
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
        else
        {
            if (severity == RS2_LOG_SEVERITY_ERROR ||
                severity == RS2_LOG_SEVERITY_WARN)
            {
                c = alpha(dark_red, 1 - t);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
            }
            else
            {
                c = alpha(saturate(grey, 0.7f), 1 - t);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, saturate(c, 1.3));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, saturate(c, 0.9));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(c, 1.5));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        c = alpha(white, 1 - t);
        ImGui::PushStyleColor(ImGuiCol_Text, c);
    }

    const int notification_model::get_max_lifetime_ms() const
    {
        return 10000;
    }

    void notification_model::draw_text(const char* msg, int x, int y, int h)
    {
        std::string text_name = to_string() << "##notification_text_" << index;
        ImGui::PushTextWrapPos(x + width - 100);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
        if (enable_click) ImGui::Text("%s", msg);
        else ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(msg),
            strlen(msg) + 1, { float(width - (count > 1 ? 40 : 10)), float(h) },
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(6);
        ImGui::PopTextWrapPos();

        if (ImGui::IsItemHovered())
        {
            last_interacted = system_clock::now();
        }
    }

    std::function<void()> notification_model::draw(ux_window& win, int w, int y, 
        notification_model& selected, std::string& error_message)
    {
        std::function<void()> follow_up = []{};

        if (visible)
        {

            auto stack = std::min(count, max_stack);
            auto x = w - width - 10;

            if (dismissed)
            {
                x = w + width;
            }

            if (!animating && (fabs(x - last_x) > 1.f || fabs(y - last_y) > 1.f))
            {
                if (last_x > 100000)
                {
                    last_x = x + 500;
                    last_y = y;
                }
                last_moved = system_clock::now();
                animating = true;
            }

            auto elapsed = duration<double, milli>(system_clock::now() - last_moved).count();
            auto s = smoothstep(static_cast<float>(elapsed / 250.f), 0.0f, 1.0f);

            if (s < 1.f)
            {
                x = s * x + (1 - s) * last_x;
                y = s * y + (1 - s) * last_y;
            }
            else
            {
                last_x = x; last_y = y;
                animating = false;
                if (dismissed && !expanded) to_close = true;
            }

            auto ms = get_age_in_ms() / get_max_lifetime_ms();
            auto t = smoothstep(static_cast<float>(ms), 0.8f, 1.f);
            if (pinned) t = 0.f;

            set_color_scheme(t);

            auto title = message;

            auto parts = split_string(title, '`');
            if (parts.size() > 1) title = parts[0];

            auto lines = static_cast<int>(std::count(title.begin(), title.end(), '\n') + 1);
            height = lines * 30 + 5;

            if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
            {
                if (update_state != 2) height = 150;
                else height = 65;
            }

            auto c = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
            c.w = smoothstep(static_cast<float>(get_age_in_ms(true) / 200.f), 0.0f, 1.0f);
            c.w = std::min(c.w, 1.f - t);

            if (update_state == 2)
            {
                auto k = duration_cast<milliseconds>(system_clock::now() - last_progress_time).count() / 500.f;
                if (k <= 1.f)
                {
                    auto size = 100;
                    k = pow(1.f - smoothstep(static_cast<float>(k), 0.f, 1.f), 2.f);
                    ImGui::GetWindowDrawList()->AddRectFilled({ float(x - size * k), float(y - size * k) },
                    { float(x + width + size * k), float(y + height + size * k) },
                        ImColor(alpha(white, (1.f - k) / 2)), (size * k) / 2);
                }
                //if (k >= 6.f) dismissed = true;
            }

            for (int i = stack - 1; i >= 0; i--)
            {
                auto ccopy = alpha(c, (0.9f * c.w) / (i + 1));

                ImVec4 shadow{ 0.1f, 0.1f, 0.1f, 0.1f };

                ImGui::GetWindowDrawList()->AddRectFilled({ float(x + 2 + i * stack_offset), float(y + 2 + i * stack_offset) },
                { float(x + 2 + width + i * stack_offset), float(y + 2 + height + i * stack_offset) }, ImColor(shadow));

                ImGui::GetWindowDrawList()->AddRectFilledMultiColor({ float(x + i  * stack_offset), float(y + i * stack_offset) },
                { float(x + width + i * stack_offset), float(y + height + i * stack_offset) },
                    ImColor(saturate(ccopy, 0.9f)), ImColor(saturate(ccopy, 0.95f)),
                    ImColor(saturate(ccopy, 1.2f)), ImColor(saturate(ccopy, 1.1f)));

                ImGui::GetWindowDrawList()->AddRect({ float(x + i * stack_offset), float(y + i * stack_offset) },
                { float(x + width + i * stack_offset), float(y + height + i * stack_offset) }, ImColor(saturate(ccopy, 0.5f)));
            }

            ImGui::SetCursorScreenPos({ float(x), float(y) });

            if (enable_click)
            {
                std::string button_name = to_string() << "##" << index;

                ImGui::PushStyleColor(ImGuiCol_Button, transparent);

                if (ImGui::Button(button_name.c_str(), { (float)width, (float)height }))
                {
                    follow_up = custom_action;
                    dismissed = true;
                }
                if (ImGui::IsItemHovered())
                    win.link_hovered();

                ImGui::PopStyleColor();
            }

            if (count > 1)
            {
                std::string count_str = to_string() << "x " << count;
                ImGui::SetCursorScreenPos({ float(x + width - 22 - count_str.size() * 5), float(y + 7) });
                ImGui::Text("%s", count_str.c_str());
            }

            ImGui::SetCursorScreenPos({ float(x + 5), float(y + 5) });

            if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
            {
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

                ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
                ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
                { float(x + width), float(y + 25) }, ImColor(shadow));

                if (update_state != 2)
                {
                    ImGui::Text("Firmware Update Recommended!");

                    ImGui::SetCursorScreenPos({ float(x + 5), float(y + 27) });

                    draw_text(title.c_str(), x, y, height - 50);

                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 67) });

                    ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1. - t));

                    if (update_state == 0)
                        ImGui::Text("Firmware updates offer critical bug fixes and\nunlock new camera capabilities.");
                    else
                        ImGui::Text("Firmware updates is underway...\nPlease do not disconnect the device");

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
                    if (update_state == 0)
                    {
                        auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                        ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                        std::string button_name = to_string() << "Install" << "##fwupdate" << index;

                        if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }) || update_manager->started())
                        {
                            if (!update_manager->started()) update_manager->start();

                            update_state = 1;
                            enable_dismiss = false;
                            last_progress_time = system_clock::now();
                        }
                        ImGui::PopStyleColor(2);

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("%s", "New firmware will be flashed to the device");
                        }
                    }
                    else if (update_state == 1)
                    {
                        if (update_manager->done())
                        {
                            update_state = 2;
                            pinned = false;
                            last_progress_time = last_interacted = system_clock::now();
                        }

                        if (!expanded)
                        {
                            if (update_manager->failed())
                            {
                                update_manager->check_error(error_message);
                                update_state = 3;
                                pinned = false;
                                dismissed = true;
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
            else
            {
                draw_text(title.c_str(), x, y, height - 35);
            }

            if (enable_expand)
            {
                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

                ImGui::PushFont(win.get_large_font());

                ImGui::PushStyleColor(ImGuiCol_Button, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
                string id = to_string() << textual_icons::dotdotdot << "##" << index;
                if (ImGui::Button(id.c_str()))
                {
                    selected = *this;
                }

                if (ImGui::IsItemHovered())
                    win.link_hovered();

                ImGui::PopStyleColor(3);
                ImGui::PopFont();
            }

            if (enable_dismiss)
            {
                ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

                string id = to_string() << "Dismiss" << "##" << index;
                if (ImGui::Button(id.c_str(), { 100, 20 }))
                {
                    dismissed = true;
                }
            }

            unset_color_scheme();
        }

        if (expanded)
        {
            if (update_manager->started() && update_state == 0) update_state = 1;

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
                            update_state = 3;
                            pinned = false;
                            dismissed = true;
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
        

        return follow_up;
    }

    int notifications_model::add_notification(const notification_data& n)
    {
        return add_notification(n, []{}, false);
    }

    int notifications_model::add_notification(const notification_data& n,
        std::function<void()> custom_action, bool use_custom_action)
    {
        int result = 0;

        {
            using namespace std;
            using namespace chrono;
            lock_guard<mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                       // done from the notifications callback and proccesing and removing of old notifications done from the main thread

            for (auto&& nm : pending_notifications)
            {
                if (nm.category == n.get_category() && nm.message == n.get_description() &&
                    nm.category != RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
                {
                    nm.last_interacted = std::chrono::system_clock::now();
                    nm.count++;
                    return nm.index;
                }
            }

            notification_model m(n);
            m.index = index++;
            result = m.index;
            m.timestamp = duration<double, milli>(system_clock::now().time_since_epoch()).count();

            if (n.get_category() == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
            {
                m.pinned = true;
                m.enable_expand = false;
            }

            if (n.get_category() == RS2_NOTIFICATION_CATEGORY_COUNT)
            {
                m.pinned = true;
            }

            if (use_custom_action)
            {
                m.custom_action = custom_action;
                m.enable_click = true;
                m.enable_expand = false;
                m.enable_dismiss = false;
            }
            
            pending_notifications.push_back(m);

            if (pending_notifications.size() > (size_t)MAX_SIZE)
            {
                auto it = pending_notifications.begin();
                while (it != pending_notifications.end() && it->pinned) it++;

                if (it != pending_notifications.end())
                    pending_notifications.erase(it);
            }
        }

        add_log(n.get_description());
        return result;
    }

    void notifications_model::draw(ux_window& win, int w, int h, std::string& error_message)
    {
        ImGui::PushFont(win.get_font());

        std::vector<std::function<void()>> follow_up;

        {
            std::lock_guard<std::mutex> lock(m);
            if (pending_notifications.size() > 0)
            {
                // loop over all notifications, remove "old" ones
                pending_notifications.erase(std::remove_if(std::begin(pending_notifications),
                    std::end(pending_notifications),
                    [&](notification_model& n)
                {
                    return ((n.get_age_in_ms() > n.get_max_lifetime_ms() && !n.pinned && !n.expanded) || n.to_close);
                }), end(pending_notifications));

                auto height = 60;
                for (auto& noti : pending_notifications)
                {
                    follow_up.push_back(noti.draw(win, w, height, selected, error_message));

                    if (noti.visible)
                        height += noti.height + 4 +
                            std::min(noti.count, noti.max_stack) * noti.stack_offset;
                }
            }

            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            //ImGui::Begin("Notification parent window", nullptr, flags);

            //selected.set_color_scheme(0.f);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

            if (selected.message != "")
                ImGui::OpenPopup("Notification from Hardware");
            if (ImGui::BeginPopupModal("Notification from Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Received the following notification:");

                auto parts = split_string(selected.message, '`');

                std::stringstream ss;
                ss << "Timestamp: "
                    << std::fixed << selected.timestamp
                    << "\nSeverity: " << selected.severity
                    << "\nDescription: ";
                    
                for (auto&& part : parts) ss << part << "\n";

                auto s = ss.str();
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                ImGui::InputTextMultiline("notification", const_cast<char*>(s.c_str()),
                    s.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    selected.message = "";
                    ImGui::CloseCurrentPopup();
                }

                std::string clip = "";
                auto lines = split_string(selected.message, '\n');
                for (auto line : lines)
                {
                    if (line.size() && line[0] == '$') clip += line.substr(2) + "\n";
                }
                if (clip != "")
                {
                    ImGui::SameLine();
                    if (ImGui::Button(" Copy Commands "))
                    {
                        glfwSetClipboardString(win, clip.c_str());
                    }
                    if (ImGui::IsItemActive())
                        ImGui::SetTooltip("Paste the copied commands to a terminal and enter your password to run");
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            //selected.unset_color_scheme();
            //ImGui::End();

            ImGui::PopStyleColor();
        }

        for (auto& action : follow_up)
        {
            try
            {
                action();
            }
            catch (...) {}
        }
        
        ImGui::PopFont();
    }

    void notifications_model::dismiss(int idx)
    {
        std::lock_guard<std::mutex> lock(m);
        for (auto& noti : pending_notifications)
        {
            if (noti.index == idx) noti.dismissed = true;
        }
    }

    void notifications_model::attach_update_manager(int idx, 
        std::shared_ptr<firmware_update_manager> manager, bool expanded)
    {
        std::lock_guard<std::mutex> lock(m);
        for (auto& noti : pending_notifications)
        {
            if (noti.index == idx)
            {
                noti.update_manager = manager;
                if (expanded)
                {
                    noti.expanded = true;
                    noti.visible = false;
                }
            }
        }
    }
}
