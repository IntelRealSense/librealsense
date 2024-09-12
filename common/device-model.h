// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <set>
#include "notifications.h"
#include "realsense-ui-advanced-mode.h"
#include <rsutils/json.h>
#include "sw-update/dev-updates-profile.h"
#include <rsutils/time/periodic-timer.h>
#include "updates-model.h"
#include "calibration-model.h"
#include "objects-in-frame.h"

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
static const ImVec4 yellowish = from_rgba(255, 253, 191, 255, true);
static const ImVec4 green = from_rgba(0x20, 0xe0, 0x20, 0xff, true);
static const ImVec4 dark_sensor_bg = from_rgba(0x1b, 0x21, 0x25, 170);
static const ImVec4 red = from_rgba(233, 0, 0, 255, true);
static const ImVec4 greenish = from_rgba(67, 163, 97, 255);
static const ImVec4 orange = from_rgba(255, 157, 0, 255, true);

inline ImVec4 operator*(const ImVec4& color, float t)
{
    return ImVec4(color.x * t, color.y * t, color.z * t, color.w * t);
}
inline ImVec4 operator+(const ImVec4& a, const ImVec4& b)
{
    return ImVec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline ImVec4 blend(const ImVec4& c, float a)
{
    return{ c.x, c.y, c.z, a * c.w };
}

namespace rs2
{
    void imgui_easy_theming(ImFont*& font_dynamic, ImFont*& font_18, ImFont*& monofont, int& font_size);

    constexpr const char* server_versions_db_url = "https://librealsense.intel.com/Releases/rs_versions_db.json";

    typedef std::vector<std::unique_ptr<device_model>> device_models_list;

    void open_issue(const device_models_list& devices);

    struct textual_icon
    {
        explicit constexpr textual_icon(const char(&unicode_icon)[4]) :
            _icon{ unicode_icon[0], unicode_icon[1], unicode_icon[2], unicode_icon[3] }
        {
        }
        operator const char* () const
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
            static const char* file_save_mode{ "record.file_save_mode" };
            static const char* default_path{ "record.default_path" };
            static const char* compression_mode{ "record.compression" };
        }
        namespace update
        {
            static const char* allow_rc_firmware{ "update.allow_rc_firmware" };
            static const char* recommend_updates{ "update.recommend_updates" };
            static const char* recommend_calibration{ "update.recommend_calibration" };
            static const char* sw_updates_url{ "update.sw_update_url" };
            static const char* sw_updates_official_server{ "update.sw_update_official_server" };
        }
        namespace calibration
        {
            static const char* enable_writing{ "calibration.enable_writing" };
        }
        namespace viewer
        {
            static const char* is_3d_view{ "viewer_model.is_3d_view" };
            static const char* ground_truth_r{ "viewer_model.ground_truth_r" };
            static const char* target_width_r{ "viewer_model.target_width_r" };
            static const char* target_height_r{ "viewer_model.target_height_r" };
            static const char* continue_with_ui_not_aligned{ "viewer_model.continue_with_ui_not_aligned" };
            static const char* continue_with_current_fw{ "viewer_model.continue_with_current_fw" };
            static const char* settings_tab{ "viewer_model.settings_tab" };
            static const char* sdk_version{ "viewer_model.sdk_version" };
            static const char* last_calib_notice{ "viewer_model.last_calib_notice" };
            static const char* is_measuring{ "viewer_model.is_measuring" };
            static const char* output_open{ "viewer_model.output_open" };
            static const char* dashboard_open{ "viewer_model.dashboard_open" };
            static const char* search_term{ "viewer_model.search_term" };

            static const char* log_to_console{ "viewer_model.log_to_console" };
            static const char* log_to_file{ "viewer_model.log_to_file" };
            static const char* log_filename{ "viewer_model.log_filename" };
            static const char* log_severity{ "viewer_model.log_severity" };
            static const char* post_processing{ "viewer_model.post_processing" };
            static const char* show_map_ruler{ "viewer_model.show_map_ruler" };
            static const char* show_stream_details{ "viewer_model.show_stream_details" };
            static const char* metric_system{ "viewer_model.metric_system" };
            static const char* shading_mode{ "viewer_model.shading_mode" };
            static const char* commands_xml{ "viewer_model.commands_xml" };
            static const char* hwlogger_xml{ "viewer_model.hwlogger_xml" };

            static const char* last_ip{ "viewer_model.last_ip" };
        }
        namespace window
        {
            static const char* is_fullscreen{ "window.is_fullscreen" };
            static const char* saved_pos{ "window.saved_pos" };
            static const char* position_x{ "window.position_x" };
            static const char* position_y{ "window.position_y" };
            static const char* saved_size{ "window.saved_size" };
            static const char* width{ "window.width" };
            static const char* height{ "window.height" };
            static const char* maximized{ "window.maximized" };
            static const char* font_size{ "window.font_size" };
        }
        namespace performance
        {
            static const char* glsl_for_rendering{ "performance.glsl_for_rendering.v2" };
            static const char* glsl_for_processing{ "performance.glsl_for_processing.v2" };
            static const char* enable_msaa{ "performance.msaa" };
            static const char* msaa_samples{ "performance.msaa_samples" };
            static const char* show_fps{ "performance.show_fps" };
            static const char* vsync{ "performance.vsync" };
            static const char* font_oversample{ "performance.font_oversample.v2" };
            static const char* show_skybox{ "performance.show_skybox" };
            static const char* occlusion_invalidation{ "performance.occlusion_invalidation" };
        }
        namespace ply
        {
            static const char* mesh{ "ply.mesh" };
            static const char* use_normals{ "ply.normals" };
            static const char* encoding{ "ply.encoding" };

            enum encoding_types
            {
                textual = 0,
                binary = 1
            };
        }
    }

    namespace textual_icons
    {
        // A note to a maintainer - preserve order when adding values to avoid duplicates
        static const textual_icon file_movie{ u8"\uf008" };
        static const textual_icon times{ u8"\uf00d" };
        static const textual_icon download{ u8"\uf019" };
        static const textual_icon refresh{ u8"\uf021" };
        static const textual_icon lock{ u8"\uf023" };
        static const textual_icon camera{ u8"\uf030" };
        static const textual_icon video_camera{ u8"\uf03d" };
        static const textual_icon edit{ u8"\uf044" };
        static const textual_icon step_backward{ u8"\uf048" };
        static const textual_icon play{ u8"\uf04b" };
        static const textual_icon pause{ u8"\uf04c" };
        static const textual_icon stop{ u8"\uf04d" };
        static const textual_icon step_forward{ u8"\uf051" };
        static const textual_icon plus_circle{ u8"\uf055" };
        static const textual_icon question_mark{ u8"\uf059" };
        static const textual_icon info_circle{ u8"\uf05a" };
        static const textual_icon fix_up{ u8"\uf062" };
        static const textual_icon minus{ u8"\uf068" };
        static const textual_icon exclamation_triangle{ u8"\uf071" };
        static const textual_icon shopping_cart{ u8"\uf07a" };
        static const textual_icon bar_chart{ u8"\uf080" };
        static const textual_icon upload{ u8"\uf093" };
        static const textual_icon square_o{ u8"\uf096" };
        static const textual_icon unlock{ u8"\uf09c" };
        static const textual_icon floppy{ u8"\uf0c7" };
        static const textual_icon square{ u8"\uf0c8" };
        static const textual_icon bars{ u8"\uf0c9" };
        static const textual_icon caret_down{ u8"\uf0d7" };
        static const textual_icon repeat{ u8"\uf0e2" };
        static const textual_icon circle{ u8"\uf111" };
        static const textual_icon check_square_o{ u8"\uf14a" };
        static const textual_icon cubes{ u8"\uf1b3" };
        static const textual_icon toggle_off{ u8"\uf204" };
        static const textual_icon toggle_on{ u8"\uf205" };
        static const textual_icon connectdevelop{ u8"\uf20e" };
        static const textual_icon usb_type{ u8"\uf287" };
        static const textual_icon braille{ u8"\uf2a1" };
        static const textual_icon window_maximize{ u8"\uf2d0" };
        static const textual_icon window_restore{ u8"\uf2d2" };
        static const textual_icon grid{ u8"\uf1cb" };
        static const textual_icon exit{ u8"\uf011" };
        static const textual_icon see_less{ u8"\uf070" };
        static const textual_icon dotdotdot{ u8"\uf141" };
        static const textual_icon link{ u8"\uf08e" };
        static const textual_icon throphy{ u8"\uF091" };
        static const textual_icon metadata{ u8"\uF0AE" };
        static const textual_icon check{ u8"\uF00C" };
        static const textual_icon mail{ u8"\uF01C" };
        static const textual_icon cube{ u8"\uf1b2" };
        static const textual_icon measure{ u8"\uf545" };
        static const textual_icon wifi{ u8"\uf1eb" };
    }

    class viewer_model;
    class ux_window;
    class subdevice_model;

    class syncer_model
    {
    public:
        syncer_model() :
            _active(true) {}

        std::shared_ptr<rs2::asynchronous_syncer> create_syncer()
        {
            stop();
            std::lock_guard<std::mutex> lock(_mutex);
            auto shared_syncer = std::make_shared<rs2::asynchronous_syncer>();

            // This queue stores the output from the syncer, and has to be large enough to deal with
            // slowdowns on the processing/rendering threads of the Viewer to avoid frame drops. Frame
            // drops can be pretty noticeable to the user!
            //
            // The syncer may also give out frames in bursts. This commonly happens, for example, when
            // streams have different FPS, and is a known issue. Even with same-FPS streams, the actual
            // FPS may change depending on a number of variables (e.g., exposure). When FPS is not the
            // same between the streams, a latency is introduced and bursts from the syncer are the
            // result.
            //
            // Bursts are so fast the other threads will never have a chance to pull them in time. For
            // example, the syncer output can be the following, all one after the other:
            //
            //     [Color: timestamp 100 (arrived @ 105), Infrared: timestamp 100 (arrived at 150)]
            //     [Color: timestamp 116 (arrived @ 120)]
            //     [Color: timestamp 132 (arrived @ 145)]
            //
            // They are received one at a time & pushed into the syncer, which will wait and keep all of
            // them inside until a match with Infrared is possible. Once that happens, it will output
            // everything it has as a burst.
            //
            // The queue size must therefore be big enough to deal with expected latency: the more
            // latency, the bigger the burst.
            //
            // Another option is to use an aggregator, similar to the behavior inside the pipeline.
            // But this still doesn't solve the issue of the bursts: we'll get frame drops in the fast
            // stream instead of the slow.
            //
            rs2::frame_queue q(10);

            _syncers.push_back({ shared_syncer,q });
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
                }), _syncers.end());
            start();
        }

        std::vector<rs2::frameset> try_wait_for_frames()
        {
            std::lock_guard<std::mutex> lock(_mutex);

            std::vector<rs2::frameset> result;
            for (auto&& s = _syncers.begin(); s != _syncers.end() && _active; s++)
            {
                rs2::frameset f;
                if (s->second.try_wait_for_frame(&f, 1))
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

        std::function<void()> on_frame = [] {};
    private:
        std::vector<std::pair<std::shared_ptr<rs2::asynchronous_syncer>, rs2::frame_queue>> _syncers;
        std::mutex _mutex;
        std::atomic<bool> _active;
    };

    class device_model
    {
    public:
        typedef std::function<void(std::function<void()> load)> json_loading_func;

        void reset();
        explicit device_model(device& dev, std::string& error_message, viewer_model& viewer, bool new_device_connected = true, bool allow_remove=true);
        ~device_model();
        void start_recording(const std::string& path, std::string& error_message);
        void stop_recording(viewer_model& viewer);
        void pause_record();
        void resume_record();

        void refresh_notifications(viewer_model& viewer);
        bool check_for_bundled_fw_update( const rs2::context & ctx,
                                          std::shared_ptr< notifications_model > not_model,
                                          bool reset_delay = false );

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
        void begin_update(std::vector<uint8_t> data,
            viewer_model& viewer, std::string& error_message);
        void begin_update_unsigned(viewer_model& viewer, std::string& error_message);
        void check_for_device_updates(viewer_model& viewer, bool activated_by_user = false);
        bool disable_record_button_logic(bool is_streaming, bool is_playback_device);
        std::string get_record_button_hover_text(bool is_streaming);


        std::shared_ptr< atomic_objects_in_frame > get_detected_objects() const { return _detected_objects; }

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
        bool _allow_remove = true;
        bool show_depth_only = false;
        bool show_stream_selection = true;
        std::vector<std::pair<std::string, std::string>> infos;
        std::vector<std::string> restarting_device_info;
        std::set<std::string> advanced_mode_settings_file_names;
        std::string selected_file_preset;

        std::vector<std::shared_ptr<notification_model>> related_notifications;

    private:
        // This class is in charge of camera accuracy health window parameters,
        // Needed as a member for reseting the window memory on device disconnection.


        void draw_info_icon(ux_window& window, ImFont* font, const ImVec2& size);
        int draw_seek_bar();
        int draw_playback_controls(ux_window& window, ImFont* font, viewer_model& view);
        advanced_mode_control amc;
        std::string pretty_time(std::chrono::nanoseconds duration);
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
            ux_window& window,
            const std::string& error_message);

        void load_viewer_configurations(const std::string& json_str);
        void save_viewer_configurations(std::ofstream& outfile, rsutils::json& j);
        void handle_online_sw_update(
            std::shared_ptr< notifications_model > nm,
            std::shared_ptr< sw_update::dev_updates_profile::update_profile > update_profile,
            bool reset_delay = false );

        bool handle_online_fw_update(
            const context & ctx,
            std::shared_ptr< notifications_model > nm,
            std::shared_ptr< sw_update::dev_updates_profile::update_profile > update_profile,
            bool reset_delay = false );

        std::shared_ptr<recorder> _recorder;
        std::vector<std::shared_ptr<subdevice_model>> live_subdevices;
        rsutils::time::periodic_timer      _update_readonly_options_timer;
        bool pause_required = false;
        std::shared_ptr< atomic_objects_in_frame > _detected_objects;
        std::shared_ptr<updates_model> _updates;
        std::shared_ptr<sw_update::dev_updates_profile::update_profile >_updates_profile;
        calibration_model _calib_model;
    };

    std::pair<std::string, std::string> get_device_name(const device& dev);

    std::vector<std::pair<std::string, std::string>> get_devices_names(const device_list& list);

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
