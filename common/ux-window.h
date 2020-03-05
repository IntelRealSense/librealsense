#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include "imgui.h"
#include <string>
#include <functional>
#include <thread>
#include "rendering.h"
#include <atomic>
#include <memory>

namespace rs2
{
    class visualizer_2d;
    class context;

    class viewer_ui_traits
    {
    public:
        const static int control_panel_width = 280;
        const static int control_panel_height = 40;
        const static int metrics_panel_width = 250;
        const static int default_log_h = 80;
        // Flags for pop-up window - no window resize, move or collaps
        const static auto imgui_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;
    };

    class ux_window
    {
    public:
        std::function<void(std::string)> on_file_drop = [](std::string) {};
        std::function<bool()>            on_load = []() { return false; };

        ux_window(const char* title, context &ctx);

        float width() const { return float(_width); }
        float height() const { return float(_height); }

        float framebuf_width() const { return float(_fb_width); }
        float framebuf_height() const { return float(_fb_height); }

        // Check that the graphic subsystem is valid and start a new frame
        operator bool();

        ~ux_window();

        operator GLFWwindow*() const { return _win; }

        void begin_frame();

        void begin_viewport();

        void end_frame();

        void reset();

        ImFont* get_large_font() const { return _font_18; }
        ImFont* get_font() const { return _font_14; }

        rs2::mouse_info& get_mouse() { return _mouse; }
        float get_scale_factor() const { return _scale_factor; }

        void add_on_load_message(const std::string& msg);

        bool is_ui_aligned() { return _is_ui_aligned; }
        bool is_fullscreen() { return _fullscreen; }

        texture_buffer& get_splash() { return _splash_tex; }

        void reload();
        void refresh();

        void link_hovered();
    private:
        void open_window();

        void setup_icon();

        void imgui_config_push();
        void imgui_config_pop();

        GLFWwindow               *_win;
        int                      _width, _height, _output_height;
        int                     _fb_width, _fb_height;
        rs2::rect                _viewer_rect;

        ImFont                   *_font_14, *_font_18;
        rs2::mouse_info          _mouse;
        std::string              _error_message;
        float                    _scale_factor;

        std::thread              _first_load;
        bool                     _first_frame;
        std::atomic<bool>        _app_ready;
        std::atomic<bool>        _keep_alive;
        texture_buffer           _splash_tex;
        timer                    _splash_timer;
        std::string              _title_str;
        std::vector<std::string> _on_load_message;
        std::mutex               _on_load_message_mtx;

        bool                     _query_devices = true;
        bool                     _missing_device = false;
        int                      _hourglass_index = 0;
        std::string              _dev_stat_message;
        bool                     _fullscreen_pressed = false;
        bool                     _fullscreen = false;
        bool                     _reload = false;
        bool                     _show_fps = false;
        bool                     _vsync = true;
        bool                     _use_glsl_proc = false;
        bool                     _use_glsl_render = false;
        bool                     _enable_msaa = false;
        int                      _msaa_samples = 0;

        bool                     _link_hovered = false;
        GLFWcursor*              _hand_cursor = nullptr;

        std::string              _title;
        std::shared_ptr<visualizer_2d> _2d_vis;
        context                  &_ctx;

        bool                     _is_ui_aligned = false;
    };
}
