// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "model-views.h"
#include "notifications.h"
#include "viewer.h"
#include <librealsense2/hpp/rs_export.hpp>

namespace rs2
{
    struct popup
    {
        const std::string header;
        const std::string message;
        std::function<void()> custom_command;

        bool operator =(const popup& p)
        {
            return p.message == message;
        }
    };

    class viewer_model;

    class frameset_allocator : public filter
    {
    public:
        frameset_allocator(viewer_model* viewer);
    private:
        viewer_model* owner;
    };

    struct export_model
    {
        template<typename T, size_t sz>
        static export_model make_exporter(std::string name, std::string extension, T (&filters_str)[sz])
        {
            return export_model(name, extension, filters_str, sz);
            
        }
        std::string name;
        std::string extension;
        std::vector<char> filters;
        std::map<rs2_option, int> options;

    private:
        export_model(std::string name, std::string extension, const char* filters_str, size_t filters_size) : name(name),
            extension(extension), filters(filters_str, filters_str + filters_size) {};
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

        viewer_model(context &ctx_);

        ~viewer_model()
        {
            // Stopping post processing filter rendering thread
            ppf.stop();
            streams.clear();
        }

        void begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p);

        std::shared_ptr<texture_buffer> get_last_texture();

        std::vector<frame> get_frames(frame set);
        frame get_3d_depth_source(frame f);
        frame get_3d_texture_source(frame f);

        bool is_3d_depth_source(frame f);
        bool is_3d_texture_source(frame f);

        std::shared_ptr<texture_buffer> upload_frame(frame&& f);

        std::map<int, rect> calc_layout(const rect& r);

        void show_no_stream_overlay(ImFont* font, int min_x, int min_y, int max_x, int max_y);
        void show_no_device_overlay(ImFont* font, int min_x, int min_y);
        void show_rendering_not_supported(ImFont* font_18, int min_x, int min_y, int max_x, int max_y, rs2_format format);

        void show_paused_icon(ImFont* font, int x, int y, int id);
        void show_recording_icon(ImFont* font_18, int x, int y, int id, float alpha_delta);

        void popup_if_error(const ux_window& window, std::string& error_message);

        void show_popup(const ux_window& window, const popup& p);

        void popup_firmware_update_progress(const ux_window& window, const float progress);

        void show_event_log(ImFont* font_14, float x, float y, float w, float h);

        void render_pose(rs2::rect stream_rect, float buttons_heights);

        void show_3dviewer_header(ImFont* large_font, ImFont* font, rs2::rect stream_rect, bool& paused, std::string& error_message);

        void update_3d_camera(ux_window& win, const rect& viewer_rect, bool force = false);

        void show_top_bar(ux_window& window, const rect& viewer_rect, const device_models_list& devices);

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

        context &ctx;
        notifications_model not_model;
        bool is_output_collapsed = false;
        bool is_3d_view = false;
        bool paused = false;
        bool metric_system = true;
        uint32_t ground_truth_r = 2500;

        enum export_type
        {
            ply
        };
        std::map<export_type, export_model> exporters;
        frameset_allocator frameset_alloc;

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
        std::atomic<int> zo_sensors;

        int selected_depth_source_uid = -1;
        int selected_tex_source_uid = -1;

        float dim_level = 1.f;

        bool continue_with_ui_not_aligned = false;
        bool continue_with_current_fw = false;

        press_button_model trajectory_button{ u8"\uf1b0", u8"\uf1b0","Draw trajectory", "Stop drawing trajectory", true };
        press_button_model grid_object_button{ u8"\uf1cb", u8"\uf1cb",  "Configure Grid", "Configure Grid", false };
        press_button_model pose_info_object_button{ u8"\uf05a", u8"\uf05a",  "Show pose stream info overlay", "Hide pose stream info overlay", false };

        bool show_pose_info_3d = false;

    private:
        void check_permissions();

        std::vector<popup> _active_popups;

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

        void set_export_popup(ImFont* large_font, ImFont* font, rect stream_rect, std::string& error_message, config_file& temp_cfg);

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

        rs2::gl::camera_renderer _cam_renderer;
        rs2::gl::pointcloud_renderer _pc_renderer;
    };
}
