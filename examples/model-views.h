// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs2.hpp>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <map>
#include <set>
#include <array>
#include "example.hpp"

#define WHITE_SPACES std::string("                                        ")

namespace rs2
{
    class subdevice_model;

    template<class T>
    void push_back_if_not_exists(std::vector<T>& vec, T value)
    {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it == vec.end()) vec.push_back(value);
    }

    struct frame_metadata
    {
        std::array<std::pair<bool,rs2_metadata_t>,RS2_FRAME_METADATA_COUNT> md_attributes{};
    };

    typedef std::map<rs2_stream, rect> streams_layout;

    class option_model
    {
    public:
        void draw(std::string& error_message);
        void update_supported(std::string& error_message);
        void update_read_only(std::string& error_message);
        void update_all(std::string& error_message);

        rs2_option opt;
        option_range range;
        sensor endpoint;
        bool* invalidate_flag;
        bool supported = false;
        bool read_only = false;
        float value = 0.0f;
        std::string label = "";
        std::string id = "";
        subdevice_model* dev;

    private:
        bool is_all_integers() const;
        bool is_enum() const;
        bool is_checkbox() const;
    };

    class subdevice_model
    {
    public:
        subdevice_model(device& dev, sensor& s, std::string& error_message);
        bool is_there_common_fps() ;
        void draw_stream_selection();
        bool is_selected_combination_supported();
        std::vector<stream_profile> get_selected_profiles();
        void stop();
        void play(const std::vector<stream_profile>& profiles);
        void update(std::string& error_message);

        template<typename T>
        bool get_default_selection_index(const std::vector<T>& values, const T & def, int* index)
        {
            auto max_default = values.begin();
            for (auto it = values.begin(); it != values.end(); it++)
            {

                if (*it == def)
                {
                    *index = (int)(it - values.begin());
                    return true;
                }
                if (*max_default < *it)
                {
                    max_default = it;
                }
            }
            *index = (int)(max_default - values.begin());
            return false;
        }

        sensor s;
        device dev;

        std::map<rs2_option, option_model> options_metadata;
        std::vector<std::string> resolutions;
        std::map<rs2_stream, std::vector<std::string>> fpses_per_stream;
        std::vector<std::string> shared_fpses;
        std::map<rs2_stream, std::vector<std::string>> formats;
        std::map<rs2_stream, bool> stream_enabled;

        int selected_res_id = 0;
        std::map<rs2_stream, int> selected_fps_id;
        int selected_shared_fps_id = 0;
        std::map<rs2_stream, int> selected_format_id;

        std::vector<std::pair<int, int>> res_values;
        std::map<rs2_stream, std::vector<int>> fps_values_per_stream;
        std::vector<int> shared_fps_values;
        bool show_single_fps_list = false;
        std::map<rs2_stream, std::vector<rs2_format>> format_values;

        std::vector<stream_profile> profiles;

        std::vector<std::unique_ptr<frame_queue>> queues;
        bool options_invalidated = false;
        int next_option = RS2_OPTION_COUNT;
        bool streaming = false;

        rect normalized_zoom{0, 0, 1, 1};
        rect roi_rect;
        bool auto_exposure_enabled = false;
        float depth_units = 1.f;

        bool roi_checked = false;
    };

    class stream_model
    {
    public:
        stream_model();
        void upload_frame(frame&& f);
        void outline_rect(const rect& r);
        float get_stream_alpha();
        bool is_stream_visible();
        void update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message);
        void show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message);
        void show_metadata(const mouse_info& g);
        rect get_normalized_zoom(const rect& stream_rect, const mouse_info& g, bool is_middle_clicked, float zoom_val);

        rect layout;
        std::unique_ptr<texture_buffer> texture;
        float2 size;
        rect get_stream_bounds() const { return { 0, 0, size.x, size.y }; }

        rs2_stream stream;
        rs2_format format;
        std::chrono::high_resolution_clock::time_point last_frame;
        double              timestamp;
        unsigned long long  frame_number;
        rs2_timestamp_domain timestamp_domain;
        fps_calc            fps;
        rect                roi_display_rect{};
        frame_metadata      frame_md;
        bool                metadata_displayed  = false;
        bool                capturing_roi       = false;    // active modification of roi
        std::shared_ptr<subdevice_model> dev;
        float _frame_timeout = 700.0f;
        float _min_timeout = 167.0f;

        bool _mid_click = false;
        float2 _middle_pos{0, 0};
        rect _normalized_zoom{0, 0, 1, 1};
        int color_map_idx = 0;
    };

    class device_model
    {
    public:
        device_model(){}
        void reset();
        explicit device_model(device& dev, std::string& error_message);
        bool draw_combo_box(const std::vector<std::string>& device_names, int& new_index);
        void draw_device_details(device& dev);
        std::map<rs2_stream, rect> calc_layout(float x0, float y0, float width, float height);
        void upload_frame(frame&& f);

        std::vector<std::shared_ptr<subdevice_model>> subdevices;
        std::map<rs2_stream, stream_model> streams;
        bool fullscreen = false;
        bool metadata_supported = false;
        rs2_stream selected_stream = RS2_STREAM_ANY;

    private:
        std::map<rs2_stream, rect> get_interpolated_layout(const std::map<rs2_stream, rect>& l);

        streams_layout _layout;
        streams_layout _old_layout;
        std::chrono::high_resolution_clock::time_point _transition_start_time;
    };


    struct notification_data
    {
        notification_data(std::string description,
                            double timestamp,
                            rs2_log_severity severity,
                            rs2_notification_category category);
        rs2_notification_category get_category() const;
        std::string get_description() const;
        double get_timestamp() const;
        rs2_log_severity get_severity() const;

        std::string _description;
        double _timestamp;
        rs2_log_severity _severity;
        rs2_notification_category _category;
    };

    struct notification_model
    {
        notification_model();
        notification_model(const notification_data& n);
        double get_age_in_ms();
        void draw(int w, int y, notification_model& selected);

        static const int MAX_LIFETIME_MS = 10000;
        int height = 60;
        int index;
        std::string message;
        double timestamp;
        rs2_log_severity severity;
        std::chrono::high_resolution_clock::time_point created_time;
        // TODO: Add more info
    };

    struct notifications_model
    {
        void add_notification(const notification_data& n);
        void draw(int w, int h, notification_model& selected);

        std::vector<notification_model> pending_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::mutex m;
    };
}
