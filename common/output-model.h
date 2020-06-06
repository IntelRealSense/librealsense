// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <functional>
#include <string>

#include "ux-window.h"
#include "rendering.h"

#include "../src/concurrency.h"

namespace rs2
{
    class output_model
    {
    public:
        struct log_entry
        {
            std::string line = "";
            std::string filename = "";
            rs2_log_severity severity = RS2_LOG_SEVERITY_FATAL;
            int line_number = 0;
            double time_added = 0.0;
            std::string timestamp = "";
            bool selected = false;
        };

        output_model();

        void add_log(rs2_log_severity severity, std::string filename, int line_number, std::string line);

        void draw(ux_window& win, rect view_rect);

        int get_output_height() const { return default_log_h; }

        void run_command(std::string command);

    private:
        void open(ux_window& win);

        void foreach_log(std::function<void(log_entry& line)> action);
        bool round_indicator(ux_window& win, std::string icon, int count, 
            ImVec4 color, std::string tooltip, bool& highlighted);

        bool new_log = false;
        std::recursive_mutex m;

        single_consumer_queue<log_entry> incoming_log_queue;
        std::deque<log_entry> notification_logs;

        animated<int> default_log_h { 36 };
        bool is_output_open = true;

        bool enable_firmware_logs = false;

        bool errors_selected = false;
        bool warnings_selected = false;
        bool info_selected = false;

        bool errors_highlighted = false;
        bool warnings_highlighted = false;
        bool info_highlighted = false;
        bool drops_highlighted = false;

        rs2_log_severity min_severity = RS2_LOG_SEVERITY_DEBUG;
        rs2_log_severity max_severity = RS2_LOG_SEVERITY_FATAL;

        int number_of_errors = 0;
        int number_of_warnings = 0;
        int number_of_info = 0;
        int number_of_leaks = 0;

        animated<int> search_width { 0, std::chrono::milliseconds(400) };
        bool search_open = false;

        std::string search_line { "" };
        std::string command_line { "" };
        std::deque<std::string> commands_histroy;
        int history_offset = 0;
    };
}