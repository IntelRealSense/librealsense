// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <glad/glad.h>
#include "output-model.h"

#include "model-views.h"
#include "os.h"

#include <imgui_internal.h>

using namespace rs2;

output_model::output_model()
{
    is_output_open = config_file::instance().get_or_default(
            configurations::viewer::output_collapsed, true);
    search_line = config_file::instance().get_or_default(
            configurations::viewer::search_term, std::string(""));
    if (search_line != "") search_open = true;
}

bool output_model::round_indicator(ux_window& win, std::string icon,
    int count, ImVec4 color, std::string tooltip, bool& highlighted)
{
    std::stringstream ss;
    ss << icon;
    if (count > 0) ss << " " << count;
    auto size = ImGui::CalcTextSize(ss.str().c_str());

    if (count == 0 || (!is_output_open && !highlighted)) {
        color = dark_sensor_bg;
        ImGui::PushStyleColor(ImGuiCol_Text, header_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, header_color);
    }
    else
    {
        if (!highlighted)
        {
            color = saturate(color, 0.3f);
            ImGui::PushStyleColor(ImGuiCol_Text, white);
        }
        else
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    }

    auto pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled({ pos.x, pos.y + 3 },
                { pos.x + size.x + 15, pos.y + 27 }, ImColor(color), 12, 15);

    auto res = ImGui::Button(ss.str().c_str(), ImVec2(size.x + 15, 28));
    if (count > 0 && ImGui::IsItemHovered())
    {
        highlighted = true;
        win.link_hovered();
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
    else highlighted = false;

    ImGui::PopStyleColor(2);

    return res;
}

void output_model::open(ux_window& win)
{
    is_output_open = true;
    config_file::instance().set(configurations::viewer::output_collapsed, true);
    default_log_h = (win.height() - 100) / 2;
    new_log = true;
}

void output_model::draw(ux_window& win, rect view_rect)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, scrollbar_bg);

    auto x = view_rect.x;
    auto y = view_rect.y;
    auto w = view_rect.w;
    auto h = view_rect.h;

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_Button, transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);

    ImGui::PushFont(win.get_font());
    ImGui::SetNextWindowPos({ x, y });
    ImGui::SetNextWindowSize({ w, h });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));

    ImGui::Begin("Output", nullptr, flags);

    ImGui::SetCursorPosX(w - 31);
    if (!is_output_open)
    {
        if (ImGui::Button(u8"\uF139", ImVec2(28, 28)))
        {
            open(win);
        }
        if (ImGui::IsItemHovered())
        {
            win.link_hovered();
            ImGui::SetTooltip("%s", "Open Debug Console Window");
        }

        if (default_log_h.value() != 36)
            default_log_h = 36;
    }
    else
    {
        if (ImGui::Button(u8"\uF13A", ImVec2(28, 28)))
        {
            is_output_open = false;
            config_file::instance().set(configurations::viewer::output_collapsed, false);
            default_log_h = 36;
            search_open = false;
        }
        if (ImGui::IsItemHovered())
        {
            win.link_hovered();
            ImGui::SetTooltip("%s", "Collapse Debug Console Window");
        }

        if (default_log_h.value() != (win.height() - 100) / 2)
            default_log_h = (win.height() - 100) / 2;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(5);

    if (errors_selected) errors_highlighted = true;
    if (round_indicator(win, u8"\uF057", number_of_errors, redish, "Instances of logged errors", errors_highlighted))
    {
        errors_selected = !errors_selected;
        open(win);
    }
    ImGui::SameLine();

    if (warnings_selected) warnings_highlighted = true;
    if (round_indicator(win, u8"\uF071", number_of_warnings, orange, "Instances of logged warnings", warnings_highlighted))
    {
        warnings_selected = !warnings_selected;
        open(win);
    }
    ImGui::SameLine();

    if (info_selected) info_highlighted = true;
    if (round_indicator(win, u8"\uF05A", number_of_info, greenish, "Instances of logged info messages", info_highlighted))
    {
        info_selected = !info_selected;
        open(win);
    }
    ImGui::SameLine();

    if (!is_output_open || search_open)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, header_color);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    }
    bool focus_search = false;
    if (ImGui::Button(u8"\uF002", ImVec2(28, 28)))
    {
        focus_search = true;
        search_open = true;
        open(win);
    }
    if (ImGui::IsItemHovered())
    {
        win.link_hovered();
        ImGui::SetTooltip("%s", "Search through logs");
    }
    ImGui::PopStyleColor(1);
    ImGui::SameLine();

    auto curr_x = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(curr_x - 5);

    std::stringstream ss;
    ss << u8"\uF043 " << number_of_leaks;
    auto size = ImGui::CalcTextSize(ss.str().c_str());

    char buff[1024];
    memcpy(buff, search_line.c_str(), search_line.size());
    buff[search_line.size()] = 0;

    auto actual_search_width = w - size.x - 100 - curr_x;
    if (focus_search) search_width = actual_search_width;

    if (search_open && search_width.value() != actual_search_width)
        search_width = actual_search_width;

    // if (is_output_open && search_width < 1)
    // {
    //     search_open = true;
    // }

    if (search_open)
    {
        ImGui::PushFont(win.get_monofont());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        ImGui::PushItemWidth(search_width);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
        if (ImGui::InputText("##SearchInLogs",buff, 1023))
        {
            search_line = buff;
            config_file::instance().set(configurations::viewer::search_term, search_line);
        }
        if (focus_search) ImGui::SetKeyboardFocusHere();
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PopStyleColor();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);
        ImGui::PopFont();
    }

    ImGui::SetCursorPosX(w - size.x - 3 * 30);

    if (enable_firmware_logs)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
    }
    else
    {
        if (is_output_open)
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        else
            ImGui::PushStyleColor(ImGuiCol_Text, header_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    }

    if (ImGui::Button(u8"\uF2DB", ImVec2(28, 28)))
    {
        enable_firmware_logs = !enable_firmware_logs;
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
    {
        win.link_hovered();
        if (enable_firmware_logs) ImGui::SetTooltip("%s", "Disable Firmware Logs");
        else ImGui::SetTooltip("%s", "Enable Firmware Logs");
    }
    ImGui::SameLine();

    if (round_indicator(win, u8"\uF043", number_of_leaks, regular_blue, "Frame drops", drops_highlighted))
    {
        open(win);
    }

    if (is_output_open)
    {
        ImGui::SetCursorPos(ImVec2(3, 35));


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, dark_sensor_bg);

        ImGui::BeginChild("##LogArea",
            ImVec2(0.7f * w - 4, h - 38 - ImGui::GetTextLineHeightWithSpacing() - 1), true,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

        const auto log_area_width = 0.7f * w - 4;

        bool copy_all = false;
        bool save_all = false;
        std::vector<std::string> output_strings;

        auto time_now = glfwGetTime();

        int i = 0;
        foreach_log([&](log_entry& log) 
        {
            auto line = log.line;
            if (log.line_number)
            {
                line = log.filename.substr(log.filename.find_last_of("/\\") + 1) + ":";
                line += std::string(to_string() << log.line_number) + " - ";
                line += log.line;
            }

            bool ok = false;
            if (info_selected || warnings_selected || errors_selected)
            {
                if (info_selected && log.severity <= RS2_LOG_SEVERITY_INFO) ok = true;
                if (warnings_selected && log.severity == RS2_LOG_SEVERITY_WARN) ok = true;
                if (errors_selected && log.severity >= RS2_LOG_SEVERITY_ERROR) ok = true;
            }
            else ok = true;

            if (search_line != "" && line.find(search_line) == std::string::npos) ok = false;

            if (!ok) return;

            std::stringstream ss; ss << log.timestamp << " [" << rs2_log_severity_to_string(log.severity) << "] ";
            if (log.line_number) ss << log.filename << ":" << log.line_number;
            ss << " - " << log.line;
            std::string full = ss.str();

            ImGui::PushFont(win.get_monofont());
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);

            ImVec4 color = redish;
            if (log.severity >= RS2_LOG_SEVERITY_ERROR)
            {
                color = redish;
            } 
            else if (log.severity >= RS2_LOG_SEVERITY_WARN)
            {
                color = orange;
            } 
            else
            {
                color = greenish;
            }

            auto margin = ImGui::GetTextLineHeightWithSpacing() - ImGui::GetTextLineHeight();
            auto size = ImGui::CalcTextSize(line.c_str());
            
            auto t = single_wave(time_now - log.time_added + 0.3f) * 0.2f;
            if (log.selected) t = 0.2f;

            auto pos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled({ pos.x, pos.y },
                        { pos.x + log_area_width, pos.y + size.y + 2 * margin }, 
                        ImColor(alpha(saturate(color, 0.3f + t), 0.7f + t)));
            ImGui::GetWindowDrawList()->AddLine({ pos.x, pos.y + size.y + 2 * margin },
                        { pos.x + log_area_width, pos.y + size.y + 2 * margin }, ImColor(alpha(color, 0.5f)));

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
            ImGui::Text("%s", log.timestamp.c_str()); ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);

            std::string label = to_string() << "##log_entry" << i++;
            ImGui::InputTextEx(label.c_str(),
                        (char*)line.data(),
                        static_cast<int>(line.size() + 1),
                        ImVec2(-1, size.y + margin),
                        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

            ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
            ImGui::PushStyleColor(ImGuiCol_Text, black);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5,5));
            label = to_string() << "##log_entry" << i << "_context_menu";
            if (ImGui::BeginPopupContextItem(label.c_str())) 
            {
                log.selected = true;
                ImGui::PushFont(win.get_font());
                if (ImGui::Selectable("Copy Line")) {
                    glfwSetClipboardString(win, full.c_str());
                }
                if (ImGui::Selectable("Copy All")) {
                    copy_all = true;
                }
                if (ImGui::Selectable("Save As...")) {
                    save_all = true;
                }
                ImGui::PopFont();
                ImGui::EndPopup();
            }
            else log.selected = false;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            ImGui::PopStyleColor(3);
            ImGui::PopFont();
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1);

            output_strings.push_back(full);
        });

        std::stringstream ss;
        for (auto&& s : output_strings) ss << s << "\n";
        if (copy_all)
            glfwSetClipboardString(win, ss.str().c_str());

        if (save_all)
        {
            if (auto fn = file_dialog_open(file_dialog_mode::save_file, "Log File\0*.log\0", nullptr, nullptr))
            {
                std::ofstream out(fn);
                if (out.good())
                {
                    out << ss.str();
                }
                out.close();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();


        ImGui::SetCursorPos(ImVec2(7, h - ImGui::GetTextLineHeightWithSpacing() - 2));
        ImGui::Text("%s", u8"\uF120"); ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(30, h - ImGui::GetTextLineHeightWithSpacing() - 4));


        ImGui::PushFont(win.get_monofont());
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
        ImGui::PushItemWidth(0.7f * w - 32);

        bool command_focus = false;
        if (ImGui::IsKeyPressed(GLFW_KEY_UP) || ImGui::IsKeyPressed(GLFW_KEY_DOWN))
        {
            if (commands_histroy.size())
            {
                if (ImGui::IsKeyPressed(GLFW_KEY_UP)) history_offset = (history_offset + 1) % commands_histroy.size();
                if (ImGui::IsKeyPressed(GLFW_KEY_DOWN)) history_offset = (history_offset - 1 + commands_histroy.size()) % commands_histroy.size();
                command_line = commands_histroy[history_offset];
            }
        }

        memcpy(buff, command_line.c_str(), command_line.size());
        buff[command_line.size()] = 0;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, scrollbar_bg);
        if (ImGui::InputText("##TerminalCommand", buff, 1023, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (!command_focus) command_line = buff;
        }
        ImGui::PopStyleColor();
        if (command_focus) ImGui::SetKeyboardFocusHere();
        ImGui::PopFont();
        ImGui::PopStyleColor();

        if (ImGui::IsKeyPressed(GLFW_KEY_ENTER) || ImGui::IsKeyPressed(GLFW_KEY_KP_ENTER))
        {
            if (commands_histroy.size() > 100) commands_histroy.pop_back();
            commands_histroy.push_front(command_line);
            run_command(command_line);
            command_line = "";
            command_focus = true;
        }

        if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE))
        {
            command_line = "";
        }

        ImGui::SetCursorPos(ImVec2(0.7f * w - 2, 35));
        ImGui::BeginChild("##StatsArea",ImVec2(0.3f * w - 2, h - 38), true);

        ImGui::EndChild();


        ImGui::PopStyleColor();
    }
    else foreach_log([&](log_entry& log) {});
    

    ImGui::End();
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

void output_model::foreach_log(std::function<void(log_entry& line)> action)
{
    std::lock_guard<std::recursive_mutex> lock(m);

    // Process only the messages that are available upon invocation
    log_entry le;
    for (size_t len = 0; len < incoming_log_queue.size(); len++)
    {
        if (incoming_log_queue.try_dequeue(&le))
        {
            if (le.severity >= RS2_LOG_SEVERITY_ERROR) number_of_errors++;
            else if (le.severity >= RS2_LOG_SEVERITY_WARN) number_of_warnings++;
            else number_of_info++;

            notification_logs.push_back(le);
        }
    }

    // Limit the notification window
    while (notification_logs.size() > 1000)
    {
        auto&& le = notification_logs.front();
        if (le.severity >= RS2_LOG_SEVERITY_ERROR) number_of_errors--;
        else if (le.severity >= RS2_LOG_SEVERITY_WARN) number_of_warnings--;
        else number_of_info--;
        notification_logs.pop_front();
    }

    for (auto&& l : notification_logs)
        action(l);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

    if (new_log)
    {
        ImGui::SetScrollPosHere();
        new_log = false;
    }
}

// Callback function must not include mutex
void output_model::add_log(rs2_log_severity severity, std::string filename, int line_number, std::string line)
{
    if (!line.size()) return;

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%H:%M:%S",timeinfo);

    log_entry e;
    e.line = line;
    e.line_number = line_number;
    e.filename = filename;
    e.severity = severity;
    e.timestamp = buffer;
    e.time_added = glfwGetTime();

    incoming_log_queue.enqueue(std::move(e));
    new_log = true;
}

void output_model::run_command(std::string command)
{
    if (to_lower(command) == "clear")
    {
        while (notification_logs.size() > 0)
        {
            auto&& le = notification_logs.front();
            if (le.severity >= RS2_LOG_SEVERITY_ERROR) number_of_errors--;
            else if (le.severity >= RS2_LOG_SEVERITY_WARN) number_of_warnings--;
            else number_of_info--;
            notification_logs.pop_front();
        }
        return;
    }

    add_log(RS2_LOG_SEVERITY_WARN, __FILE__, __LINE__, to_string() << "Unrecognized command '" << command << "'");
}