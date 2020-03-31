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

#include "model-views.h"

#include "os.h"

#include "viewer.h"

#include "metadata-helper.h"

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

        created_time = std::chrono::system_clock::now();
        last_interacted = std::chrono::system_clock::now() - std::chrono::milliseconds(500);
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

    // Pops the N colors that were pushed in set_color_scheme
    void notification_model::unset_color_scheme() const
    {
        ImGui::PopStyleColor(6);
    }

    void process_notification_model::draw_progress_bar(ux_window & win, int bar_width)
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

            threshold_progress = float(std::min(100, progress + delta));

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

            rs2::rect pbar{ float(pos.x + 3), float(pos.y + 3), float(bar_width), 17.f };
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

        ImGui::PushStyleColor(ImGuiCol_Button, saturate(c, 1.3));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, saturate(c, 0.9));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(c, 1.5));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        c = alpha(white, 1 - t);
        ImGui::PushStyleColor(ImGuiCol_Text, c);

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

    std::string notification_model::get_title()
    {
        auto title = message;

        auto parts = split_string(title, '`');
        if (parts.size() > 1) title = parts[0];

        return title;
    }

    int notification_model::calc_height()
    {
        auto title = get_title();
        auto lines = static_cast<int>(std::count(title.begin(), title.end(), '\n') + 1);
        return (lines + 1) * ImGui::GetTextLineHeight() + 5;
    }

    void process_notification_model::draw_pre_effect(int x, int y)
    {
        // TODO: Make polymorphic
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
        }
    }

    void notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        draw_text(get_title().c_str(), x, y, height - 35);
    }

    void rs2::notification_model::draw_dismiss(ux_window & win, int x, int y)
    {
        ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

        string id = to_string() << "Dismiss" << "##" << index;
        if (ImGui::Button(id.c_str(), { 100, 20 }))
        {
            dismiss(true);
        }
    }

    std::function<void()> notification_model::draw(ux_window& win, int w, int y, 
        std::shared_ptr<notification_model>& selected, std::string& error_message)
    {
        std::function<void()> action;
        while(dispatch_queue.try_dequeue(&action)) action();
        
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

            height = calc_height();

            auto c = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
            c.w = smoothstep(static_cast<float>(get_age_in_ms(true) / 200.f), 0.0f, 1.0f);
            c.w = std::min(c.w, 1.f - t);

            draw_pre_effect(x, y);

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
                    dismiss(false);
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

            draw_content(win, x, y, t, error_message);

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
                    selected = shared_from_this();
                }

                if (ImGui::IsItemHovered())
                    win.link_hovered();

                ImGui::PopStyleColor(3);
                ImGui::PopFont();
            }

            if (enable_dismiss)
            {
                draw_dismiss(win, x, y);
            }
            
            unset_color_scheme();
        }

        if (expanded)
        {
            draw_expanded(win, error_message);
        }
        

        return follow_up;
    }

    std::shared_ptr<notification_model> notifications_model::add_notification(const notification_data& n)
    {
        return add_notification(n, []{}, false);
    }

    std::shared_ptr<notification_model> notifications_model::add_notification(const notification_data& n,
        std::function<void()> custom_action, bool use_custom_action)
    {
        std::shared_ptr<notification_model> result = nullptr;
        {
            using namespace std;
            using namespace chrono;
            lock_guard<recursive_mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                       // done from the notifications callback and proccesing and removing of old notifications done from the main thread

            for (auto&& nm : pending_notifications)
            {
                if (nm->category == n.get_category() && nm->message == n.get_description())
                {
                    nm->last_interacted = std::chrono::system_clock::now();
                    nm->count++;
                    return nm;
                }
            }

            auto m = std::make_shared<notification_model>(n);
            m->index = index++;
            result = m;
            m->timestamp = duration<double, milli>(system_clock::now().time_since_epoch()).count();

            if (n.get_category() == RS2_NOTIFICATION_CATEGORY_COUNT)
            {
                m->pinned = true;
            }

            if (use_custom_action)
            {
                m->custom_action = custom_action;
                m->enable_click = true;
                m->enable_expand = false;
                m->enable_dismiss = false;
            }
            
            pending_notifications.push_back(m);

            if (pending_notifications.size() > (size_t)MAX_SIZE)
            {
                auto it = pending_notifications.begin();
                while (it != pending_notifications.end() && (*it)->pinned) it++;

                if (it != pending_notifications.end())
                    pending_notifications.erase(it);
            }
        }

        add_log(n.get_description());
        return result;
    }

    void notifications_model::add_notification(std::shared_ptr<notification_model> model)
    {
        {
            using namespace std;
            using namespace chrono;
            lock_guard<recursive_mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                       // done from the notifications callback and proccesing and removing of old notifications done from the main thread

            model->index = index++;
            model->timestamp = duration<double, milli>(system_clock::now().time_since_epoch()).count();

            pending_notifications.push_back(model);

            if (pending_notifications.size() > (size_t)MAX_SIZE)
            {
                auto it = pending_notifications.begin();
                while (it != pending_notifications.end() && (*it)->pinned) it++;

                if (it != pending_notifications.end())
                    pending_notifications.erase(it);
            }
        }

        add_log(model->get_title());
    }

    void notifications_model::draw_snoozed_button()
    {
        auto has_snoozed = snoozed_notifications.size();
        ImGui::PushStyleColor(ImGuiCol_Text, !has_snoozed ? sensor_bg : light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, !has_snoozed ? sensor_bg : light_blue);

        const auto width = 50.f;

        using namespace std;
        using namespace chrono;

        if (!has_snoozed) 
        {
            ImGui::ButtonEx(textual_icons::mail, { width, width }, ImGuiButtonFlags_Disabled);

            if (ImGui::IsItemActive())
                ImGui::SetTooltip("No pending notifications at this point");
        }
        else
        {
            auto k = duration_cast<milliseconds>(system_clock::now() - last_snoozed).count() / 500.f;
            if (k <= 1.f)
            {
                auto size = 50.f;

                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, saturate(white, smoothstep(static_cast<float>(k), 0.f, 1.f)));
            }

            if (ImGui::Button(textual_icons::mail, { width, width }))
            {
                for (auto&& n : snoozed_notifications)
                {
                    n->forced = true;
                    n->snoozed = false;
                    n->last_y -= 500;
                    pending_notifications.push_back(n);
                }
                snoozed_notifications.clear();
            }

            if (ImGui::IsItemActive())
                ImGui::SetTooltip("Pending notifications available. Click to review");
        }

        ImGui::PopStyleColor(2);
    }

    void notifications_model::draw(ux_window& win, int w, int h, std::string& error_message)
    {
        ImGui::PushFont(win.get_font());

        std::vector<std::function<void()>> follow_up;

        {
            bool pinned_drawn = false;
            std::lock_guard<std::recursive_mutex> lock(m);
            if (pending_notifications.size() > 0)
            {
                snoozed_notifications.erase(std::remove_if(std::begin(snoozed_notifications),
                    std::end(snoozed_notifications),
                    [&](std::shared_ptr<notification_model>& n)
                {
                    return n->dismissed;
                }), end(snoozed_notifications));

                // loop over all notifications, remove "old" ones
                pending_notifications.erase(std::remove_if(std::begin(pending_notifications),
                    std::end(pending_notifications),
                    [&](std::shared_ptr<notification_model>& n)
                {
                    if (n->snoozed && n->pinned)
                    {
                        n->dismissed = false;
                        n->to_close = false;
                        snoozed_notifications.push_back(n);
                        last_snoozed = std::chrono::system_clock::now();
                        return true;
                    }
                    return ((n->get_age_in_ms() > n->get_max_lifetime_ms() && 
                            !n->pinned && !n->expanded) || n->to_close);
                }), end(pending_notifications));

                auto height = 60;
                for (auto& noti : pending_notifications)
                {
                    if (pinned_drawn && noti->pinned && !noti->forced) 
                    {
                        continue;
                    }

                    follow_up.push_back(noti->draw(win, w, height, selected, error_message));

                    if (noti->pinned) pinned_drawn = true;

                    if (noti->visible)
                        height += noti->height + 4 +
                            std::min(noti->count, noti->max_stack) * noti->stack_offset;
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

            if (selected && selected->message != "")
                ImGui::OpenPopup("Notification from Hardware");
            if (ImGui::BeginPopupModal("Notification from Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Received the following notification:");

                auto parts = split_string(selected->message, '`');

                std::stringstream ss;
                ss << "Timestamp: "
                    << std::fixed << selected->timestamp
                    << "\nSeverity: " << selected->severity
                    << "\nDescription: ";
                    
                for (auto&& part : parts) ss << part << "\n";

                auto s = ss.str();
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                ImGui::InputTextMultiline("notification", const_cast<char*>(s.c_str()),
                    s.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    selected->message = "";
                    selected = nullptr;
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    std::string clip = "";
                    auto lines = split_string(selected->message, '\n');
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
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);

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

    void notifications_model::foreach_log(std::function<void(const std::string& line)> action)
    {
        std::lock_guard<std::recursive_mutex> lock(m);
        for (auto&& l : log)
        {
            action(l);
        }

        auto rc = ImGui::GetCursorPos();
        ImGui::SetCursorPos({ rc.x, rc.y + 5 });

        if (new_log)
        {
            ImGui::SetScrollPosHere();
            new_log = false;
        }
    }

    void notifications_model::add_log(std::string line)
    {
        std::lock_guard<std::recursive_mutex> lock(m);
        if (!line.size()) return;
        // Limit the notification window
        while (log.size() > 200)
            log.pop_front();
        if (line[line.size() - 1] != '\n') line += "\n";
        log.push_back(line);
        new_log = true;
    }

    version_upgrade_model::version_upgrade_model(int version) 
        : process_notification_model(nullptr), _version(version)
    {
        enable_expand = false;
        enable_dismiss = true;
        update_state = 2;
        //pinned = true;
        last_progress_time = system_clock::now();
    } 

    void version_upgrade_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);
        ImGui::PopStyleColor(1);
        auto c = alpha(sensor_bg, 1 - t);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
    }

    void version_upgrade_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        if (_first)
        {
            last_progress_time = system_clock::now();
            _first = false;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1.f - t));
        ImGui::PushFont(win.get_large_font());

        ImGui::SetCursorScreenPos({ float(x + 20), float(y + 16) });
        ImGui::Text("Welcome to"); ImGui::SameLine();
        std::string txt = to_string() << "librealsense " << RS2_API_VERSION_STR << "!";

        ImGui::PushStyleColor(ImGuiCol_Text, alpha(white, 1.f - t));
        ImGui::Text("%s", txt.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::SetCursorScreenPos({ float(x + 17), float(y + 41) });

        std::string link = to_string() << "https://github.com/IntelRealSense/librealsense/wiki/Release-Notes#release-" << _version; 

        ImGui::PushStyleColor(ImGuiCol_Text, alpha(white, 1.f - t));
        if (ImGui::Button("What's new"))
        {
            open_url(link.c_str());
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            win.link_hovered();
            ImGui::SetTooltip("Open the Release Notes. Internet connection is required");
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5);
        ImGui::Text("in this release?");
        ImGui::PopStyleColor();
    }

    int version_upgrade_model::calc_height()
    {
        return 80;
    }

    void process_manager::log(std::string line)
    {
        std::lock_guard<std::mutex> lock(_log_lock);
        _log += line + "\n";
    }

    void process_manager::reset()
    {
        _progress = 0;
        _started = false;
        _done = false;
        _failed = false;
        _last_error = "";
    }

    void process_manager::fail(std::string error)
    {
        _last_error = error;
        _progress = 0;
        log("\nERROR: " + error);
        _failed = true;
    }

    void notification_model::invoke(std::function<void()> action)
    {
        single_consumer_queue<bool> q;
        dispatch_queue.enqueue([&q, &action](){
            try
            {
                action();
                q.enqueue(true);
            }
            catch(...)
            {
                q.enqueue(false);
            }
        });
        bool res;
        if (!q.dequeue(&res, 100000) || !res)
            throw std::runtime_error("Invoke operation failed!");
    }

    void process_manager::start(std::shared_ptr<notification_model> n)
    {
        auto cleanup = [n]() {
            //n->dismiss(false);
        };

        auto invoke = [n](std::function<void()> action) {
            n->invoke(action);
        };

        log(to_string() << "Started " << _process_name << " process");

        auto me = shared_from_this();
        std::weak_ptr<process_manager> ptr(me);

        std::thread t([ptr, cleanup, invoke]() {
            auto self = ptr.lock();
            if (!self) return;

            try
            {
                self->process_flow(cleanup, invoke);
            }
            catch (const error& e)
            {
                self->fail(error_to_string(e));
                cleanup();
            }
            catch (const std::exception& ex)
            {
                self->fail(ex.what());
                cleanup();
            }
            catch (...)
            {
                self->fail(to_string() << "Unknown error during " << self->_process_name << " process!");
                cleanup();
            }
        });
        t.detach();

        _started = true;
    }

    void export_manager::process_flow(
        std::function<void()> cleanup,
        invoker invoke)
    {
        _progress = 5;
        _exporter->process(_data);
        _progress = 100;

        _done = true;
    }

    void export_notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        using namespace std;
        using namespace chrono;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
        { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != STATE_COMPLETE)
        {
            ImGui::Text("Export in progress");

            ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1. - t));

            std::string s = to_string() << "Saving 3D view to " <<
                get_file_name(get_manager().get_filename());
            ImGui::Text("%s", s.c_str());

            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Text("Export Completed");

            ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });
            ImGui::PushFont(win.get_large_font());
            std::string txt = to_string() << textual_icons::throphy;
            ImGui::Text("%s", txt.c_str());
            ImGui::PopFont();

            ImGui::SetCursorScreenPos({ float(x + 40), float(y + 35) });
            std::string s = to_string() << "Finished saving 3D view  to " <<
                get_file_name(get_manager().get_filename());

            ImGui::Text("%s", s.c_str());
        }

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        const auto bar_width = width - 115;

        if (update_state == STATE_IN_PROGRESS)
        {
            if (update_manager->done())
            {
                update_state = STATE_COMPLETE;
                pinned = false;
                last_progress_time = last_interacted = system_clock::now();
            }

            if (!expanded)
            {
                if (update_manager->failed())
                {
                    update_manager->check_error(error_message);
                    update_state = STATE_FAILED;
                    pinned = false;
                    dismiss(false);
                }

                draw_progress_bar(win, bar_width);

                ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });
            }
        }
    }

    int export_notification_model::calc_height()
    {
        return 85;
    }

    void export_notification_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);

        ImGui::PopStyleColor(1);

        ImVec4 c;

        if (update_state == STATE_COMPLETE)
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

    export_notification_model::export_notification_model(std::shared_ptr<export_manager> manager)
        : process_notification_model(manager)
    {
        enable_expand = false;
        expanded = false;
        if (expanded) visible = false;

        message = "";
        update_state = STATE_IN_PROGRESS;
        this->severity = RS2_LOG_SEVERITY_INFO;
        this->category = RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED;

        pinned = true;
    }

    metadata_warning_model::metadata_warning_model()
        : notification_model()
    {
        enable_expand = false;
        enable_dismiss = true;
        pinned = true;
        message = "Frame Metadata is a device feature allowing\n"
                  "software synchronization between different\n"
                  "camera streams.\n"
                  "It must be explicitly enabled on Windows OS\n";
    }

    void metadata_warning_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);
        ImGui::PopStyleColor(1);
        auto c = alpha(sensor_bg, 1 - t);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
    }

    void metadata_warning_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
        { float(x + width), float(y + 25) }, ImColor(shadow));

        ImGui::Text("Frame Metadata Disabled");

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + 27) });

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        draw_text(get_title().c_str(), x, y, height - 50);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
        ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
        std::string button_name = to_string() << "Enable" << "##enable_metadata" << index;

        const auto bar_width = width - 115;
        if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
        {
            metadata_helper::instance().enable_metadata();
            dismiss(false);
        }
        ImGui::PopStyleColor(2);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", "Enables metadata on connected devices (you may be prompted for administrator privileges)");
        }
    }
}
