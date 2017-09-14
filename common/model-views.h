// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense2/rs.hpp>

#include "rendering.h"
#include "parser.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <map>
#include <set>
#include <array>
#include <unordered_map>

#include "imgui-fonts-karla.hpp"
#include "imgui-fonts-fontawesome.hpp"

#include "realsense-ui-advanced-mode.h"


inline ImVec4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ImVec4(r / (float)255, g / (float)255, b / (float)255, a / (float)255);
}

inline ImVec4 operator+(const ImVec4& c, float v)
{
    return ImVec4(
        std::max(0.f, std::min(1.f, c.x + v)),
        std::max(0.f, std::min(1.f, c.y + v)),
        std::max(0.f, std::min(1.f, c.z + v)),
        std::max(0.f, std::min(1.f, c.w))
    );
}


static const ImVec4 light_blue = from_rgba(0, 174, 239, 255); // Light blue color for selected elements such as play button glyph when paused
static const ImVec4 regular_blue = from_rgba(0, 115, 200, 255); // Checkbox mark, slider grabber
static const ImVec4 light_grey = from_rgba(0xc3, 0xd5, 0xe5, 0xff); // Text
static const ImVec4 dark_window_background = from_rgba(9, 11, 13, 255);
static const ImVec4 almost_white_bg = from_rgba(230, 230, 230, 255);
static const ImVec4 black = from_rgba(0, 0, 0, 255);
static const ImVec4 transparent = from_rgba(0, 0, 0, 0);
static const ImVec4 white = from_rgba(0xff, 0xff, 0xff, 0xff);
static const ImVec4 scrollbar_bg = from_rgba(14, 17, 20, 255);
static const ImVec4 scrollbar_grab = from_rgba(54, 66, 67, 255);
static const ImVec4 grey{ 0.5f,0.5f,0.5f,1.f };
static const ImVec4 dark_grey = from_rgba(30, 30, 30, 255);
static const ImVec4 sensor_header_light_blue = from_rgba(80, 99, 115, 0xff);
static const ImVec4 sensor_bg = from_rgba(36, 44, 51, 0xff);
static const ImVec4 redish = from_rgba(255, 46, 54, 255);
static const ImVec4 button_color = from_rgba(62, 77, 89, 0xff);
static const ImVec4 header_window_bg = from_rgba(36, 44, 54, 0xff);
static const ImVec4 header_color = from_rgba(62, 77, 89, 255);
static const ImVec4 title_color = from_rgba(27, 33, 38, 255);
static const ImVec4 device_info_color = from_rgba(33, 40, 46, 255);

void imgui_easy_theming(ImFont*& font_14, ImFont*& font_18);

namespace rs2
{
    class subdevice_model;
    struct notifications_model;

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

    struct frame_metadata
    {
        std::array<std::pair<bool,rs2_metadata_type>,RS2_FRAME_METADATA_COUNT> md_attributes{};
    };

    struct notification_model;
    typedef std::map<int, rect> streams_layout;

    std::vector<std::pair<std::string, std::string>> get_devices_names(const device_list& list);
    std::vector<std::string> get_device_info(const device& dev, bool include_location = true);

    class option_model
    {
    public:
        bool draw(std::string& error_message);
        void update_supported(std::string& error_message);
        void update_read_only_status(std::string& error_message);
        void update_all_feilds(std::string& error_message, notifications_model& model);

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

    class frame_queues
    {
    public:
        frame_queue& at(int id)
        {
            std::lock_guard<std::mutex> lock(_lookup_mutex);
            if (_queues.find(id) == _queues.end())
            {
                _queues[id] = frame_queue(4);
            }
            return _queues[id];
        }

        template<class T>
        void foreach(T action)
        {
            std::lock_guard<std::mutex> lock(_lookup_mutex);
            for (auto&& kvp : _queues)
                action(kvp.second);
        }

    private:
        std::unordered_map<int, frame_queue> _queues;
        std::mutex _lookup_mutex;
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
        void update(std::string& error_message, notifications_model& model);
        void draw_options(const std::vector<rs2_option>& drawing_order,
                          bool update_read_only_options, std::string& error_message,
                          notifications_model& model);
        bool draw_option(rs2_option opt, bool update_read_only_options,
                         std::string& error_message, notifications_model& model);


        bool is_paused() const;
        void pause();
        void resume();

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

        std::map<int, option_model> options_metadata;
        std::vector<std::string> resolutions;
        std::map<int, std::vector<std::string>> fpses_per_stream;
        std::vector<std::string> shared_fpses;
        std::map<int, std::vector<std::string>> formats;
        std::map<int, bool> stream_enabled;
        std::map<int, std::string> stream_display_names;

        int selected_res_id = 0;
        std::map<int, int> selected_fps_id;
        int selected_shared_fps_id = 0;
        std::map<int, int> selected_format_id;

        std::vector<std::pair<int, int>> res_values;
        std::map<int, std::vector<int>> fps_values_per_stream;
        std::vector<int> shared_fps_values;
        bool show_single_fps_list = false;
        std::map<int, std::vector<rs2_format>> format_values;

        std::vector<stream_profile> profiles;

        frame_queues queues;
        std::mutex _queue_lock;
        bool options_invalidated = false;
        int next_option = RS2_OPTION_COUNT;
        bool streaming = false;
        bool rgb_rotation_btn = false;

        rect normalized_zoom{0, 0, 1, 1};
        rect roi_rect;
        bool auto_exposure_enabled = false;
        float depth_units = 1.f;

        bool roi_checked = false;

        std::atomic<bool> _pause;
    };

    class viewer_model;

    inline bool ends_with(const std::string& s, const std::string& suffix)
    {
        auto i = s.rbegin(), j = suffix.rbegin();
        for (; i != s.rend() && j != suffix.rend() && *i == *j;
            i++, j++);
        return j == suffix.rend();
    }

    void outline_rect(const rect& r);
    void draw_rect(const rect& r);

    class stream_model
    {
    public:
        stream_model();
        void upload_frame(frame&& f);
        float get_stream_alpha();
        bool is_stream_visible();
        void update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message);
        void show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message);
        void show_metadata(const mouse_info& g);
        rect get_normalized_zoom(const rect& stream_rect, const mouse_info& g, bool is_middle_clicked, float zoom_val);

        void show_stream_footer(rect stream_rect, mouse_info& mouse);
        void show_stream_header(ImFont* font, rs2::rect stream_rect, viewer_model& viewer);

        rect layout;
        std::unique_ptr<texture_buffer> texture;
        float2 size;
        rect get_stream_bounds() const { return { 0, 0, size.x, size.y }; }

        stream_profile profile;
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
        int color_map_idx = 1;
        bool show_stream_details = false;

    };

    std::pair<std::string, std::string> get_device_name(const device& dev);

    bool draw_combo_box(const std::string& id, const std::vector<std::string>& device_names, int& new_index);

    class device_model
    {
    public:
        void reset();
        explicit device_model(device& dev, std::string& error_message, viewer_model& viewer);
        void draw_device_details(device& dev, context& ctx);
        void start_recording(const std::string& path, std::string& error_message);
        void stop_recording();
        void pause_record();
        void resume_record();
        int draw_playback_panel(ImFont* font, viewer_model& view);
        void draw_advanced_mode_tab(device& dev, std::vector<std::string>& restarting_info);

        std::vector<std::shared_ptr<subdevice_model>> subdevices;

        bool metadata_supported = false;
        bool get_curr_advanced_controls = true;
        device dev;
        std::string id;
        bool is_recording = false;
        int seek_pos = 0;
        int playback_speed_index = 2;
        bool _playback_repeat = true;
        bool _should_replay = false;
        bool show_device_info = false;

        std::vector<std::pair<std::string, std::string>> infos;
    private:
        int draw_seek_bar();
        int draw_playback_controls(ImFont* font, viewer_model& view);
        advanced_mode_control amc;
        std::string pretty_time(std::chrono::nanoseconds duration);
        
        void play_defaults(viewer_model& view);

        std::shared_ptr<recorder> _recorder;
        std::vector<std::shared_ptr<subdevice_model>> live_subdevices;
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
        double get_age_in_ms() const;
        void draw(int w, int y, notification_model& selected);
        void set_color_scheme(float t) const;

        static const int MAX_LIFETIME_MS = 10000;
        int height = 40;
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
        void draw(ImFont* font, int w, int h);

        std::string get_log()
        {
            std::string result;
            std::lock_guard<std::mutex> lock(m);
            for (auto&& l : log) std::copy(l.begin(), l.end(), std::back_inserter(result));
            return result;
        }

        void add_log(std::string line)
        {
            std::lock_guard<std::mutex> lock(m);
            if (!line.size()) return;
            if (line[line.size() - 1] != '\n') line += "\n";
            line = "- " + line;
            log.push_back(line);
        }

        std::vector<notification_model> pending_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::mutex m;

        std::vector<std::string> log;
        notification_model selected;
    };

    std::string get_file_name(const std::string& path);

    class async_pointclound_mapper
    {
    public:
        async_pointclound_mapper()
            : keep_calculating_pointcloud(true),
              resulting_3d_models(1), depth_frames_to_render(1),
              t([this]() {render_loop(); })
        {
        }

        ~async_pointclound_mapper() { stop(); }

        void update_texture(frame f) { pc.map_to(f); }

        void stop()
        {
            if (keep_calculating_pointcloud)
            {
                keep_calculating_pointcloud = false;
                t.join();
            }
        }

        void push_frame(frame f)
        {
            depth_frames_to_render.enqueue(f);
        }

        points get_points()
        {
            frame f;
            if (resulting_3d_models.poll_for_frame(&f))
            {
                model = f;
            }
            return model;
        }

    private:
        void render_loop();

        pointcloud pc;
        points model;
        std::atomic<bool> keep_calculating_pointcloud;
        frame_queue depth_frames_to_render;
        frame_queue resulting_3d_models;

        std::thread t;

        int last_frame_number = 0;
        double last_timestamp = 0;
        int last_stream_id = 0;
    };

    class viewer_model
    {
    public:
        void reset_camera(float3 pos = { 0.0f, 0.0f, -1.5f });

        viewer_model()
        {
            reset_camera();
            rs2_error* e = nullptr;
            not_model.add_log(to_string() << "librealsense version: " << api_version_to_string(rs2_get_api_version(&e)) << "\n");
        }

        void upload_frame(frame&& f);
        void draw_histogram_options(float depth_scale, const subdevice_model& sensor);

        std::map<int, rect> calc_layout(const rect& r);

        void show_no_stream_overlay(ImFont* font, int min_x, int min_y, int max_x, int max_y);
        void show_no_device_overlay(ImFont* font, int min_x, int min_y);

        void show_paused_icon(ImFont* font, int x, int y, int id);
        void show_recording_icon(ImFont* font_18, int x, int y, int id, float alpha_delta);

        void popup_if_error(ImFont* font, std::string& error_message);

        void show_event_log(ImFont* font_14, float x, float y, float w, float h);

        void show_3dviewer_header(ImFont* font, rs2::rect stream_rect, bool& paused);

        void update_3d_camera(const rect& viewer_rect,
                              mouse_info& mouse, bool force = false);

        void render_3d_view(const rect& view_rect, float scale_factor);

        void gc_streams();

        std::map<int, stream_model> streams;
        bool fullscreen = false;
        stream_model* selected_stream = nullptr;

        async_pointclound_mapper pc;

        notifications_model not_model;
        bool is_output_collapsed = false;
    private:
        std::map<int, rect> get_interpolated_layout(const std::map<int, rect>& l);
        void show_icon(ImFont* font_18, const char* label_str, const char* text, int x, int y, int id, const ImVec4& color);

        int selected_depth_source_uid = -1;
        int selected_tex_source_uid = -1;

        streams_layout _layout;
        streams_layout _old_layout;
        std::chrono::high_resolution_clock::time_point _transition_start_time;

        // 3D-Viewer state
        float3 pos = { 0.0f, 0.0f, -0.5f };
        float3 target = { 0.0f, 0.0f, 0.0f };
        float3 up;
        float view[16];
        bool texture_wrapping_on = true;
        GLint texture_border_mode = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER
    };

    void export_to_ply(notifications_model& ns, points points, video_frame texture);
}

