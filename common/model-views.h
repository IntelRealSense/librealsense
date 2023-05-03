// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense2/rs.hpp>

#include "rendering.h"
#include "ux-window.h"
#include "parser.hpp"
#include "rs-config.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include "opengl3.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <map>
#include <set>
#include <array>
#include <unordered_map>

#include "objects-in-frame.h"
#include "processing-block-model.h"

#include "realsense-ui-advanced-mode.h"
#include "fw-update-helper.h"
#include "updates-model.h"
#include "calibration-model.h"
#include <rsutils/time/periodic-timer.h>
#include "option-model.h"


namespace rs2
{
    void prepare_config_file();

    bool frame_metadata_to_csv( const std::string & filename, rs2::frame frame );

    bool motion_data_to_csv( const std::string & filename, rs2::frame frame );

    bool pose_data_to_csv( const std::string & filename, rs2::frame frame );

    void open_issue(std::string body);

    class option_model;

    void hyperlink(ux_window& window, const char* title, const char* link);

    static const float FEET_TO_METER = 0.3048f;

    rs2_sensor_mode resolution_from_width_height(int width, int height);

    template<class T>
    void sort_together(std::vector<T>& vec, std::vector<std::string>& names)
    {
        std::vector<std::pair<T, std::string>> pairs(vec.size());
        for (size_t i = 0; i < vec.size(); i++) pairs[i] = std::make_pair(vec[i], names[i]);

        std::sort(begin(pairs), end(pairs),
        [](const std::pair<T, std::string>& lhs,
           const std::pair<T, std::string>& rhs) {
            return lhs.first < rhs.first;
        });

        for (size_t i = 0; i < vec.size(); i++)
        {
            vec[i] = pairs[i].first;
            names[i] = pairs[i].second;
        }
    }

    template<class T>
    void push_back_if_not_exists(std::vector<T>& vec, T value)
    {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it == vec.end()) vec.push_back(value);
    }

    struct notification_model;
    typedef std::map<int, rect> streams_layout;

    std::vector<std::string> get_device_info(const device& dev, bool include_location = true);

    using color = std::array<float, 3>;
    using face = std::array<float3, 4>;
    using colored_cube = std::array<std::pair<face, color>, 6>;
    using tracked_point = std::pair<rs2_vector, unsigned int>; // translation and confidence

    class press_button_model
    {
    public:
        press_button_model(const char* icon_default, const char* icon_pressed, std::string tooltip_default, std::string tooltip_pressed,
                           bool init_pressed)
        {
            state_pressed = init_pressed;
            tooltip[unpressed] = tooltip_default;
            tooltip[pressed] = tooltip_pressed;
            icon[unpressed] = icon_default;
            icon[pressed] = icon_pressed;
        }

        void toggle_button() { state_pressed = !state_pressed; }
        void set_button_pressed(bool p) { state_pressed = p; }
        bool is_pressed() { return state_pressed; }
        std::string get_tooltip() { return(state_pressed ? tooltip[pressed] : tooltip[unpressed]); }
        std::string get_icon() { return(state_pressed ? icon[pressed] : icon[unpressed]); }

    private:
        enum button_state
        {
            unpressed, //default
            pressed
        };

        bool state_pressed = false;
        std::string tooltip[2];
        std::string icon[2];
    };

    bool yes_no_dialog(const std::string& title, const std::string& message_text, bool& approved, ux_window& window, const std::string& error_message, bool disabled = false, const std::string& disabled_reason = "");
    bool status_dialog(const std::string& title, const std::string& process_topic_text, const std::string& process_status_text, bool enable_close, ux_window& window);

    struct notifications_model;
    void export_frame(const std::string& fname, std::unique_ptr<rs2::filter> exporter, notifications_model& ns, rs2::frame data, bool notify = true);

    // Auxillary function to save stream data in its internal (raw) format
    bool save_frame_raw_data(const std::string& filename, rs2::frame frame);
}
