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

#include "imgui-fonts-karla.hpp"
#include "imgui-fonts-fontawesome.hpp"
#include "../third-party/json.hpp"

#include "realsense-ui-advanced-mode.h"

ImVec4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool consistent_color = false);
ImVec4 operator+(const ImVec4& c, float v);

static const ImVec4 light_blue = from_rgba(0, 174, 239, 255, true); // Light blue color for selected elements such as play button glyph when paused
static const ImVec4 regular_blue = from_rgba(0, 115, 200, 255, true); // Checkbox mark, slider grabber
static const ImVec4 light_grey = from_rgba(0xc3, 0xd5, 0xe5, 0xff, true); // Text
static const ImVec4 dark_window_background = from_rgba(9, 11, 13, 255);
static const ImVec4 almost_white_bg = from_rgba(230, 230, 230, 255, true);
static const ImVec4 black = from_rgba(0, 0, 0, 255, true);
static const ImVec4 transparent = from_rgba(0, 0, 0, 0, true);
static const ImVec4 white = from_rgba(0xff, 0xff, 0xff, 0xff, true);
static const ImVec4 scrollbar_bg = from_rgba(14, 17, 20, 255);
static const ImVec4 scrollbar_grab = from_rgba(54, 66, 67, 255);
static const ImVec4 grey{ 0.5f,0.5f,0.5f,1.f };
static const ImVec4 dark_grey = from_rgba(30, 30, 30, 255);
static const ImVec4 sensor_header_light_blue = from_rgba(80, 99, 115, 0xff);
static const ImVec4 sensor_bg = from_rgba(36, 44, 51, 0xff);
static const ImVec4 redish = from_rgba(255, 46, 54, 255, true);
static const ImVec4 light_red = from_rgba(255, 146, 154, 255, true);
static const ImVec4 dark_red = from_rgba(200, 46, 54, 255, true);
static const ImVec4 button_color = from_rgba(62, 77, 89, 0xff);
static const ImVec4 header_window_bg = from_rgba(36, 44, 54, 0xff);
static const ImVec4 header_color = from_rgba(62, 77, 89, 255);
static const ImVec4 title_color = from_rgba(27, 33, 38, 255);
static const ImVec4 device_info_color = from_rgba(33, 40, 46, 255);
static const ImVec4 yellow = from_rgba(229, 195, 101, 255, true);
static const ImVec4 green = from_rgba(0x20, 0xe0, 0x20, 0xff, true);
static const ImVec4 dark_sensor_bg = from_rgba(0x1b, 0x21, 0x25, 200);
static const ImVec4 red = from_rgba(233, 0, 0, 255, true);

inline ImVec4 blend(const ImVec4& c, float a)
{
    return{ c.x, c.y, c.z, a * c.w };
}

namespace rs2
{
    void prepare_config_file();

    bool frame_metadata_to_csv(const std::string& filename, rs2::frame frame);

    void open_issue(std::string body);

    struct textual_icon
    {
        explicit constexpr textual_icon(const char (&unicode_icon)[4]) :
            _icon{ unicode_icon[0], unicode_icon[1], unicode_icon[2], unicode_icon[3] }
        {
        }
        operator const char*() const
        {
            return _icon.data();
        }
    private:
        std::array<char, 5> _icon;
    };

    inline std::ostream& operator<<(std::ostream& os, const textual_icon& i)
    {
        return os << static_cast<const char*>(i);
    }

    namespace configurations
    {
        namespace record
        {
            static const char* file_save_mode      { "record.file_save_mode" };
            static const char* default_path        { "record.default_path" };
            static const char* compression_mode    { "record.compression" };
        }
        namespace viewer
        {
            static const char* is_3d_view          { "viewer_model.is_3d_view" };
            static const char* continue_with_ui_not_aligned { "viewer_model.continue_with_ui_not_aligned" };
            static const char* settings_tab        { "viewer_model.settings_tab" };

            static const char* log_to_console      { "viewer_model.log_to_console" };
            static const char* log_to_file         { "viewer_model.log_to_file" };
            static const char* log_filename        { "viewer_model.log_filename" };
            static const char* log_severity        { "viewer_model.log_severity" };
            static const char* post_processing     { "viewer_model.post_processing" };
        }
        namespace window
        {
            static const char* is_fullscreen       { "window.is_fullscreen" };
            static const char* saved_pos           { "window.saved_pos" };
            static const char* position_x          { "window.position_x" };
            static const char* position_y          { "window.position_y" };
            static const char* saved_size          { "window.saved_size" };
            static const char* width               { "window.width" };
            static const char* height              { "window.height" };
            static const char* maximized           { "window.maximized" };
        }
        namespace performance
        {
            static const char* glsl_for_rendering  { "performance.glsl_for_rendering" };
            static const char* glsl_for_processing { "performance.glsl_for_processing" };
            static const char* enable_msaa         { "performance.msaa" };
            static const char* msaa_samples        { "performance.msaa_samples" };
            static const char* show_fps            { "performance.show_fps" };
            static const char* vsync               { "performance.vsync" };
            static const char* font_oversample     { "performance.font_oversample" };
        }
    }

    namespace textual_icons
    {
        // A note to a maintainer - preserve order when adding values to avoid duplicates
        static const textual_icon file_movie               { u8"\uf008" };
        static const textual_icon times                    { u8"\uf00d" };
        static const textual_icon download                 { u8"\uf019" };
        static const textual_icon refresh                  { u8"\uf021" };
        static const textual_icon lock                     { u8"\uf023" };
        static const textual_icon camera                   { u8"\uf030" };
        static const textual_icon video_camera             { u8"\uf03d" };
        static const textual_icon edit                     { u8"\uf044" };
        static const textual_icon check_square_o           { u8"\uf046" };
        static const textual_icon step_backward            { u8"\uf048" };
        static const textual_icon play                     { u8"\uf04b" };
        static const textual_icon pause                    { u8"\uf04c" };
        static const textual_icon stop                     { u8"\uf04d" };
        static const textual_icon step_forward             { u8"\uf051" };
        static const textual_icon plus_circle              { u8"\uf055" };
        static const textual_icon question_mark            { u8"\uf059" };
        static const textual_icon info_circle              { u8"\uf05a" };
        static const textual_icon fix_up                   { u8"\uf062" };
        static const textual_icon minus                    { u8"\uf068" };
        static const textual_icon exclamation_triangle     { u8"\uf071" };
        static const textual_icon shopping_cart            { u8"\uf07a" };
        static const textual_icon bar_chart                { u8"\uf080" };
        static const textual_icon upload                   { u8"\uf093" };
        static const textual_icon square_o                 { u8"\uf096" };
        static const textual_icon unlock                   { u8"\uf09c" };
        static const textual_icon floppy                   { u8"\uf0c7" };
        static const textual_icon square                   { u8"\uf0c8" };
        static const textual_icon bars                     { u8"\uf0c9" };
        static const textual_icon caret_down               { u8"\uf0d7" };
        static const textual_icon repeat                   { u8"\uf0e2" };
        static const textual_icon circle                   { u8"\uf111" };
        static const textual_icon cubes                    { u8"\uf1b3" };
        static const textual_icon toggle_off               { u8"\uf204" };
        static const textual_icon toggle_on                { u8"\uf205" };
        static const textual_icon connectdevelop           { u8"\uf20e" };
        static const textual_icon usb_type                 { u8"\uf287" };
        static const textual_icon braille                  { u8"\uf2a1" };
        static const textual_icon window_maximize          { u8"\uf2d0" };
        static const textual_icon window_restore           { u8"\uf2d2" };
        static const textual_icon grid                     { u8"\uf1cb" };
        static const textual_icon exit                     { u8"\uf011" };
    }

    class subdevice_model;
    struct notifications_model;

    void imgui_easy_theming(ImFont*& font_14, ImFont*& font_18);

    // avoid display the following options
    bool static skip_option(rs2_option opt)
    {
        if (opt == RS2_OPTION_STREAM_FILTER ||
            opt == RS2_OPTION_STREAM_FORMAT_FILTER ||
            opt == RS2_OPTION_STREAM_INDEX_FILTER ||
            opt == RS2_OPTION_FRAMES_QUEUE_SIZE)
            return true;
        return false;
    }

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
        bool draw(std::string& error_message, notifications_model& model,bool new_line=true, bool use_option_name = true);
        void update_supported(std::string& error_message);
        void update_read_only_status(std::string& error_message);
        void update_all_fields(std::string& error_message, notifications_model& model);

        bool draw_option(bool update_read_only_options, bool is_streaming,
            std::string& error_message, notifications_model& model);

        rs2_option opt;
        option_range range;
        std::shared_ptr<options> endpoint;
        bool* invalidate_flag = nullptr;
        bool supported = false;
        bool read_only = false;
        float value = 0.0f;
        std::string label = "";
        std::string id = "";
        subdevice_model* dev;
        std::function<bool(option_model&, std::string&, notifications_model&)> custom_draw_method = nullptr;
        bool edit_mode = false;
        std::string edit_value = "";
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

    // Preserve user selections in UI
    struct subdevice_ui_selection
    {
        int selected_res_id = 0;
        int selected_shared_fps_id = 0;
        std::map<int, int> selected_fps_id;
        std::map<int, int> selected_format_id;
    };

    class viewer_model;
    class subdevice_model;

    void save_processing_block_to_config_file(const char* name, 
        std::shared_ptr<rs2::processing_block> pb, bool enable = true);

    class processing_block_model
    {
    public:
        processing_block_model(subdevice_model* owner,
            const std::string& name,
            std::shared_ptr<rs2::filter> block,
            std::function<rs2::frame(rs2::frame)> invoker,
            std::string& error_message,
            bool enabled = true);

        const std::string& get_name() const { return _name; }

        option_model& get_option(rs2_option opt);

        rs2::frame invoke(rs2::frame f) const { return _invoker(f); }

        void save_to_config_file();

        std::vector<rs2_option> get_option_list()
        {
            return _block->get_supported_options();
        }

        void populate_options(const std::string& opt_base_label,
            subdevice_model* model,
            bool* options_invalidated,
            std::string& error_message);

        std::shared_ptr<rs2::filter> get_block() { return _block; }

        bool enabled = true;
        bool visible = true;
    private:
        std::shared_ptr<rs2::filter> _block;
        std::map<int, option_model> options_metadata;
        std::string _name;
        std::string _full_name;
        std::function<rs2::frame(rs2::frame)> _invoker;
        subdevice_model* _owner;
    };

    class syncer_model
    {
    public:
        syncer_model():
        _active(true){}

        std::shared_ptr<rs2::asynchronous_syncer> create_syncer()
        {
            stop();
            std::lock_guard<std::mutex> lock(_mutex);
            auto shared_syncer = std::make_shared<rs2::asynchronous_syncer>();
            rs2::frame_queue q;

           _syncers.push_back({shared_syncer,q});
           shared_syncer->start([this, q](rs2::frame f)
           {
               q.enqueue(f);
               on_frame();
           });
           start();
           return shared_syncer;
        }

        void remove_syncer(std::shared_ptr<rs2::asynchronous_syncer> s)
        {
            stop();
            std::lock_guard<std::mutex> lock(_mutex);
            _syncers.erase(std::remove_if(_syncers.begin(), _syncers.end(),
                           [s](std::pair<std::shared_ptr<rs2::asynchronous_syncer>, rs2::frame_queue> pair)
            {
                return pair.first.get() == s.get();
            }), _syncers.end()) ;
            start();
        }

        std::vector<rs2::frameset> try_wait_for_frames()
        {
            std::lock_guard<std::mutex> lock(_mutex);

            std::vector<rs2::frameset> result;
            for(auto&& s = _syncers.begin(); s!=_syncers.end() && _active; s++)
            {
                rs2::frameset f;
                if(s->second.try_wait_for_frame(&f,1))
                {
                    result.push_back(f);
                }
            }

            return result;
        }

        void stop()
        {
            _active.exchange(false);
        }

        void start()
        {
            _active.exchange(true);
        }

        std::function<void()> on_frame = []{};
    private:
        std::vector<std::pair<std::shared_ptr<rs2::asynchronous_syncer>, rs2::frame_queue>> _syncers;
        std::mutex _mutex;
        std::atomic<bool> _active;
    };

    option_model create_option_mode(rs2_option opt, std::shared_ptr<options> options, const std::string& opt_base_label, bool* options_invalidated, std::string& error_message);

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

    class tm2_model
    {
    public:
        tm2_model() : _trajectory_tracking(true)
        {   
        }
        void draw_controller_pose_object();
        void draw_trajectory(bool is_trajectory_button_pressed);
        void update_model_trajectory(const pose_frame& pose, bool track);
        void record_trajectory(bool on) { _trajectory_tracking = on; };
        void reset_trajectory() { trajectory.clear(); };

    private:
        void add_to_trajectory(tracked_point& p);

        const float len_x = 0.1f;
        const float len_y = 0.03f;
        const float len_z = 0.01f;
        const float lens_radius = 0.005f;
        /*
        4--------------------------3
        /|                         /|
        5-|------------------------6 |
        | /1                       | /2
        |/                         |/
        7--------------------------8
        */
        float3 v1{ -len_x / 2, -len_y / 2,  len_z / 2 };
        float3 v2{ len_x / 2, -len_y / 2,  len_z / 2 };
        float3 v3{ len_x / 2,  len_y / 2,  len_z / 2 };
        float3 v4{ -len_x / 2,  len_y / 2,  len_z / 2 };
        float3 v5{ -len_x / 2,  len_y / 2, -len_z / 2 };
        float3 v6{ len_x / 2,  len_y / 2, -len_z / 2 };
        float3 v7{ -len_x / 2, -len_y / 2, -len_z / 2 };
        float3 v8{ len_x / 2, -len_y / 2, -len_z / 2 };
        face f1{ { v1,v2,v3,v4 } }; //Back
        face f2{ { v2,v8,v6,v3 } }; //Right side
        face f3{ { v4,v3,v6,v5 } }; //Top side
        face f4{ { v1,v4,v5,v7 } }; //Left side
        face f5{ { v7,v8,v6,v5 } }; //Front
        face f6{ { v1,v2,v8,v7 } }; //Bottom side

        std::array<color, 6> colors{ {
            { { 0.5f, 0.5f, 0.5f } }, //Back
        { { 0.7f, 0.7f, 0.7f } }, //Right side
        { { 1.0f, 0.7f, 0.7f } }, //Top side
        { { 0.7f, 0.7f, 0.7f } }, //Left side
        { { 0.4f, 0.4f, 0.4f } }, //Front
        { { 0.7f, 0.7f, 0.7f } }  //Bottom side
            } };

        colored_cube camera_box{ { { f1,colors[0] },{ f2,colors[1] },{ f3,colors[2] },{ f4,colors[3] },{ f5,colors[4] },{ f6,colors[5] } } };
        float3 center_left{ v5.x + len_x / 3, v6.y - len_y / 3, v5.z };
        float3 center_right{ v6.x - len_x / 3, v6.y - len_y / 3, v5.z };

        std::vector<tracked_point> trajectory;
        std::vector<float2> boundary;
        bool                _trajectory_tracking;

    };

    class subdevice_model
    {
    public:
        static void populate_options(std::map<int, option_model>& opt_container,
            const std::string& opt_base_label,
            subdevice_model* model,
            std::shared_ptr<options> options,
            bool* options_invalidated,
            std::string& error_message);

        subdevice_model(device& dev, std::shared_ptr<sensor> s, std::string& error_message);
        bool is_there_common_fps() ;
        bool draw_stream_selection();
        bool is_selected_combination_supported();
        std::vector<stream_profile> get_selected_profiles();
        void stop(viewer_model& viewer);
        void play(const std::vector<stream_profile>& profiles, viewer_model& viewer, std::shared_ptr<rs2::asynchronous_syncer>);
        bool is_synchronized_frame(viewer_model& viewer, const frame& f);
        void update(std::string& error_message, notifications_model& model);
        void draw_options(const std::vector<rs2_option>& drawing_order,
                          bool update_read_only_options, std::string& error_message,
                          notifications_model& model);
        uint64_t num_supported_non_default_options() const;
        bool draw_option(rs2_option opt, bool update_read_only_options,
            std::string& error_message, notifications_model& model)
        {
            return options_metadata[opt].draw_option(update_read_only_options, streaming, error_message, model);
        }

        bool is_paused() const;
        void pause();
        void resume();

        bool can_enable_zero_order();
        void verify_zero_order_conditions();

        void restore_ui_selection() { ui = last_valid_ui; }
        void store_ui_selection() { last_valid_ui = ui; }

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

        std::function<void()> on_frame = []{};
        std::shared_ptr<sensor> s;
        device dev;
        tm2_model tm2;

        std::map<int, option_model> options_metadata;
        std::vector<std::string> resolutions;
        std::map<int, std::vector<std::string>> fpses_per_stream;
        std::vector<std::string> shared_fpses;
        std::map<int, std::vector<std::string>> formats;
        std::map<int, bool> stream_enabled;
        std::map<int, std::string> stream_display_names;

        subdevice_ui_selection ui;
        subdevice_ui_selection last_valid_ui;

        std::vector<std::pair<int, int>> res_values;
        std::map<int, std::vector<int>> fps_values_per_stream;
        std::vector<int> shared_fps_values;
        bool show_single_fps_list = false;
        std::map<int, std::vector<rs2_format>> format_values;

        std::vector<stream_profile> profiles;

        frame_queues queues;
        std::mutex _queue_lock;
        bool _options_invalidated = false;
        int next_option = RS2_OPTION_COUNT;
        bool streaming = false;

        rect normalized_zoom{0, 0, 1, 1};
        rect roi_rect;
        bool auto_exposure_enabled = false;
        float depth_units = 1.f;
        float stereo_baseline = -1.f;

        bool roi_checked = false;

        std::atomic<bool> _pause;
        std::atomic<bool> _is_being_recorded{ false };

        bool draw_streams_selector = true;
        bool draw_fps_selector = true;

        region_of_interest algo_roi;
        bool show_algo_roi = false;

        std::shared_ptr<rs2::colorizer> depth_colorizer;
        std::shared_ptr<rs2::yuy_decoder> yuy2rgb;
        std::shared_ptr<processing_block_model> zero_order_artifact_fix;

        std::vector<std::shared_ptr<processing_block_model>> post_processing;
        bool post_processing_enabled = true;
        std::vector<std::shared_ptr<processing_block_model>> const_effects;
    };

    class viewer_model;

    void outline_rect(const rect& r);
    void draw_rect(const rect& r, int line_width = 1);

    class stream_model
    {
    public:
        stream_model();
        std::shared_ptr<texture_buffer> upload_frame(frame&& f);
        bool is_stream_visible();
        void update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message);
        void show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message);
        void show_metadata(const mouse_info& g);
        rect get_normalized_zoom(const rect& stream_rect, const mouse_info& g, bool is_middle_clicked, float zoom_val);

        bool is_stream_alive();

        void show_stream_footer(ImFont* font, const rect& stream_rect,const mouse_info& mouse);
        void show_stream_header(ImFont* font, const rect& stream_rect, viewer_model& viewer);
        void show_stream_imu(ImFont* font, const rect& stream_rect, const rs2_vector& axis);
        void show_stream_pose(ImFont* font, const rect& stream_rect, const rs2_pose& pose_data, rs2_stream stream_type, bool fullScreen, float y_offset);

        void snapshot_frame(const char* filename,viewer_model& viewer) const;

        void begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p);
        rect layout;
        std::shared_ptr<texture_buffer> texture;
        float2 size;
        float2 original_size;
        rect get_stream_bounds() const { return { 0, 0, size.x, size.y };}
        rect get_original_stream_bounds() const { return{ 0, 0, original_size.x, original_size.y };}
        stream_profile original_profile;
        stream_profile profile;
        std::chrono::high_resolution_clock::time_point last_frame;
        double              timestamp = 0.0;
        unsigned long long  frame_number = 0;
        rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        fps_calc            fps;
        rect                roi_display_rect{};
        frame_metadata      frame_md;
        bool                metadata_displayed  = false;
        bool                capturing_roi       = false;    // active modification of roi
        std::shared_ptr<subdevice_model> dev;
        float _frame_timeout = 5000.0f;
        float _min_timeout = 167.0f;

        bool _mid_click = false;
        float2 _middle_pos{0, 0};
        rect _normalized_zoom{0, 0, 1, 1};
        int color_map_idx = 1;
        bool show_stream_details = false;
        rect curr_info_rect{};
        temporal_event _stream_not_alive;
        bool show_map_ruler = true;
    };

    std::pair<std::string, std::string> get_device_name(const device& dev);

    bool draw_combo_box(const std::string& id, const std::vector<std::string>& device_names, int& new_index);

    class device_model
    {
    public:
        typedef std::function<void(std::function<void()> load)> json_loading_func;

        void reset();
        explicit device_model(device& dev, std::string& error_message, viewer_model& viewer);
        void start_recording(const std::string& path, std::string& error_message);
        void stop_recording(viewer_model& viewer);
        void pause_record();
        void resume_record();
        int draw_playback_panel(ux_window& window, ImFont* font, viewer_model& view);
        bool draw_advanced_controls(viewer_model& view, ux_window& window, std::string& error_message);
        void draw_controls(float panel_width, float panel_height,
            ux_window& window,
            std::string& error_message,
            device_model*& device_to_remove,
            viewer_model& viewer, float windows_width,
            std::vector<std::function<void()>>& draw_later,
            bool load_json_if_streaming = false,
            json_loading_func json_loading = [](std::function<void()> load) {load(); },
            bool draw_device_outline = true);
        void handle_hardware_events(const std::string& serialized_data);

        std::vector<std::shared_ptr<subdevice_model>> subdevices;
        std::shared_ptr<syncer_model> syncer;
        std::shared_ptr<rs2::asynchronous_syncer> dev_syncer;
        bool is_streaming() const;
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
        bool allow_remove = true;
        bool show_depth_only = false;
        bool show_stream_selection = true;
        std::map<int, std::array<uint8_t, 6>> controllers;
        std::set<std::array<uint8_t, 6>> available_controllers;
        std::vector<std::pair<std::string, std::string>> infos;
        std::vector<std::string> restarting_device_info;
        std::set<std::string> advanced_mode_settings_file_names;
        std::string selected_file_preset;
    private:
        void draw_info_icon(ux_window& window, ImFont* font, const ImVec2& size);
        int draw_seek_bar();
        int draw_playback_controls(ux_window& window, ImFont* font, viewer_model& view);
        advanced_mode_control amc;
        std::string pretty_time(std::chrono::nanoseconds duration);
        void draw_controllers_panel(ImFont* font, bool is_device_streaming);
        float draw_device_panel(float panel_width,
                                ux_window& window,
                                std::string& error_message,
                                viewer_model& viewer);
        void play_defaults(viewer_model& view);
        float draw_preset_panel(float panel_width,
            ux_window& window,
            std::string& error_message,
            viewer_model& viewer,
            bool update_read_only_options,
            bool load_json_if_streaming,
            json_loading_func json_loading);
        bool prompt_toggle_advanced_mode(bool enable_advanced_mode, const std::string& message_text,
            std::vector<std::string>& restarting_device_info,
            viewer_model& view,
            ux_window& window);
        void load_viewer_configurations(const std::string& json_str);
        void save_viewer_configurations(std::ofstream& outfile, nlohmann::json& j);

        std::shared_ptr<recorder> _recorder;
        std::vector<std::shared_ptr<subdevice_model>> live_subdevices;
        periodic_timer      _update_readonly_options_timer;
        bool pause_required = false;
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
        void unset_color_scheme() const;
        const int get_max_lifetime_ms() const;

        int height = 40;
        int index = 0;
        std::string message;
        double timestamp = 0.0;
        rs2_log_severity severity = RS2_LOG_SEVERITY_NONE;
        std::chrono::high_resolution_clock::time_point created_time;
        rs2_notification_category category;
        bool to_close = false; // true when user clicks on close notification
        // TODO: Add more info
    };

    struct notifications_model
    {
        void add_notification(const notification_data& n);
        void draw(ImFont* font, int w, int h);

        void foreach_log(std::function<void(const std::string& line)> action)
        {
            std::lock_guard<std::mutex> lock(m);
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

        void add_log(std::string line)
        {
            std::lock_guard<std::mutex> lock(m);
            if (!line.size()) return;
            if (line[line.size() - 1] != '\n') line += "\n";
            log.push_back(line);
            new_log = true;
        }

    private:
        std::vector<notification_model> pending_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::mutex m;
        bool new_log = false;

        std::vector<std::string> log;
        notification_model selected;
    };

    class viewer_model;
    class post_processing_filters
    {
    public:
        post_processing_filters(viewer_model& viewer)
            : processing_block([&](rs2::frame f, const rs2::frame_source& source)
            {
                process(std::move(f),source);
            }),
            viewer(viewer),
            depth_stream_active(false),
            resulting_queue_max_size(20),
            resulting_queue(static_cast<unsigned int>(resulting_queue_max_size)),
            render_thread(),
            render_thread_active(false),
            pc(new pointcloud())
        {
            std::string s;
            pc_gen = std::make_shared<processing_block_model>(nullptr, "Pointcloud Engine", pc, [=](rs2::frame f) { return pc->calculate(f); }, s);
            processing_block.start(resulting_queue);
        }

        ~post_processing_filters() { stop(); }

        void update_texture(frame f) { pc->map_to(f); }

        /* Start the rendering thread in case its disabled */
        void start();

        /* Stop the rendering thread in case its enabled */
        void stop();

        bool is_rendering() const
        {
            return render_thread_active.load();
        }

        rs2::frameset get_points()
        {
            frame f;
            if (resulting_queue.poll_for_frame(&f))
            {
                rs2::frameset frameset(f);
                model = frameset;
            }
            return model;
        }

        void reset(void)
        {
            rs2::frame f{};
            model = f;
            while(resulting_queue.poll_for_frame(&f));
        }

        std::atomic<bool> depth_stream_active;

        const size_t resulting_queue_max_size;
        std::map<int, rs2::frame_queue> frames_queue;
        rs2::frame_queue resulting_queue;

        std::shared_ptr<pointcloud> get_pc() const { return pc; }
        std::shared_ptr<processing_block_model> get_pc_model() const {  return pc_gen; }

    private:
        viewer_model& viewer;
        void process(rs2::frame f, const rs2::frame_source& source);
        std::vector<rs2::frame> handle_frame(rs2::frame f, const rs2::frame_source& source);

        void map_id(rs2::frame new_frame, rs2::frame old_frame);
        void map_id_frameset_to_frame(rs2::frameset first, rs2::frame second);
        void map_id_frameset_to_frameset(rs2::frameset first, rs2::frameset second);
        void map_id_frame_to_frame(rs2::frame first, rs2::frame second);

        rs2::frame apply_filters(rs2::frame f, const rs2::frame_source& source);
        std::shared_ptr<subdevice_model> get_frame_origin(const rs2::frame& f);

        void zero_first_pixel(const rs2::frame& f);
        rs2::frame last_tex_frame;
        rs2::processing_block processing_block;
        std::shared_ptr<pointcloud> pc;
        rs2::frameset model;
        std::shared_ptr<processing_block_model> pc_gen;

        /* Post processing filter rendering */
        std::atomic<bool> render_thread_active; // True when render post processing filter rendering thread is active, False otherwise
        std::thread render_thread;              // Post processing filter rendering Thread running render_loop()
        void render_loop();                     // Post processing filter rendering function

        int last_frame_number = 0;
        double last_timestamp = 0;
        int last_stream_id = 0;
    };

    class viewer_model
    {
    public:
        void reset_camera(float3 pos = { 0.0f, 0.0f, -1.0f });

        void update_configuration();

        const float panel_width = 340.f;
        const float panel_y = 50.f;
        const float default_log_h = 110.f;

        float get_output_height() const { return (is_output_collapsed ? default_log_h : 15); }
          
        rs2::frame handle_ready_frames(const rect& viewer_rect, ux_window& window, int devices, std::string& error_message);

        viewer_model();

        ~viewer_model()
        {
            // Stopping post processing filter rendering thread
            ppf.stop();
            streams.clear();
        }

        void begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p);

        std::vector<frame> get_frames(frame set);
        frame get_3d_depth_source(frame f);
        frame get_3d_texture_source(frame f);

        bool is_3d_depth_source(frame f);
        bool is_3d_texture_source(frame f);

        std::shared_ptr<texture_buffer> upload_frame(frame&& f);

        std::map<int, rect> calc_layout(const rect& r);

        void show_no_stream_overlay(ImFont* font, int min_x, int min_y, int max_x, int max_y);
        void show_no_device_overlay(ImFont* font, int min_x, int min_y);

        void show_paused_icon(ImFont* font, int x, int y, int id);
        void show_recording_icon(ImFont* font_18, int x, int y, int id, float alpha_delta);

        void popup_if_error(ImFont* font, std::string& error_message);

        void popup_if_ui_not_aligned(ImFont* font);

        void show_event_log(ImFont* font_14, float x, float y, float w, float h);

        void render_pose(rs2::rect stream_rect, float buttons_heights, ImGuiWindowFlags flags);

        void show_3dviewer_header(ImFont* font, rs2::rect stream_rect, bool& paused, std::string& error_message);

        void update_3d_camera(ux_window& win, const rect& viewer_rect, bool force = false);

        void show_top_bar(ux_window& window, const rect& viewer_rect, const std::vector<device_model>& devices);

        void render_3d_view(const rect& view_rect, 
            std::shared_ptr<texture_buffer> texture, rs2::points points, ImFont *font1);

        void render_2d_view(const rect& view_rect, ux_window& win, int output_height,
            ImFont *font1, ImFont *font2, size_t dev_model_num, const mouse_info &mouse, std::string& error_message);

        void gc_streams();

        std::mutex streams_mutex;
        std::map<int, stream_model> streams;
        std::map<int, int> streams_origin;
        bool fullscreen = false;
        stream_model* selected_stream = nullptr;
        std::shared_ptr<syncer_model> syncer;
        post_processing_filters ppf;

        notifications_model not_model;
        bool is_output_collapsed = false;
        bool is_3d_view = false;
        bool paused = false;


        void draw_viewport(const rect& viewer_rect, 
            ux_window& window, int devices, std::string& error_message, 
            std::shared_ptr<texture_buffer> texture, rs2::points  f = rs2::points());

        bool allow_3d_source_change = true;
        bool allow_stream_close = true;

        std::array<float3, 4> roi_rect;
        bool draw_plane = false;

        bool draw_frustrum = true;
        bool support_non_syncronized_mode = true;
        std::atomic<bool> synchronization_enable;

        int selected_depth_source_uid = -1;
        int selected_tex_source_uid = -1;

        float dim_level = 1.f;

        bool continue_with_ui_not_aligned = false;

        press_button_model trajectory_button{ u8"\uf1b0", u8"\uf1b0","Draw trajectory", "Stop drawing trajectory", true };
        press_button_model grid_object_button{ u8"\uf1cb", u8"\uf1cb",  "Configure Grid", "Configure Grid", false };
        press_button_model pose_info_object_button{ u8"\uf05a", u8"\uf05a",  "Show pose stream info overlay", "Hide pose stream info overlay", false };

        bool show_pose_info_3d = false;

    private:
        struct rgb {
            uint32_t r, g, b;
        };

        struct rgb_per_distance {
            float depth_val;
            rgb rgb_val;
        };

        friend class post_processing_filters;
        std::map<int, rect> get_interpolated_layout(const std::map<int, rect>& l);
        void show_icon(ImFont* font_18, const char* label_str, const char* text, int x, int y,
                       int id, const ImVec4& color, const std::string& tooltip = "");
        void draw_color_ruler(const mouse_info& mouse,
                              const stream_model& s_model,
                              const rect& stream_rect,
                              std::vector<rgb_per_distance> rgb_per_distance_vec,
                              float ruler_length,
                              const std::string& ruler_units);
        float calculate_ruler_max_distance(const std::vector<float>& distances) const;

        streams_layout _layout;
        streams_layout _old_layout;
        std::chrono::high_resolution_clock::time_point _transition_start_time;

        // 3D-Viewer state
        float3 pos = { 0.0f, 0.0f, -0.5f };
        float3 target = { 0.0f, 0.0f, 0.0f };
        float3 up;
        bool fixed_up = true;
        bool render_quads = true;

        float view[16];
        bool texture_wrapping_on = true;
        GLint texture_border_mode = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER

        rs2::points last_points;
        std::shared_ptr<texture_buffer> last_texture;

        // Infinite pan / rotate feature:
        bool manipulating = false;
        float2 overflow = { 0.f, 0.f };

        // Camera models
        std::vector<obj_mesh> camera_mesh;
        const int t265_mesh_id = 3;
        void render_camera_mesh(int id);
    };

    void export_to_ply(const std::string& file_name, notifications_model& ns, points p, video_frame texture, bool notify = true);

    // Auxillary function to save stream data in its internal (raw) format
    bool save_frame_raw_data(const std::string& filename, rs2::frame frame);

    // Auxillary function to store frame attributes
    bool save_frame_meta_data(const std::string& filename, rs2::frame frame);

    class device_changes
    {
    public:
        explicit device_changes(rs2::context& ctx);
        bool try_get_next_changes(event_information& removed_and_connected);
    private:
        void add_changes(const event_information& c);
        std::queue<event_information> _changes;
        std::mutex _mtx;
    };
}
