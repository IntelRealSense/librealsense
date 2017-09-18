#pragma once

#ifndef GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLU
#endif // !GLFW_INCLUDE_GLU

#include <GLFW/glfw3.h>
#include <model-views.h>

namespace rs2_depth_quality
{
    class viewer_ui_traits
    {
    public:
        const static int panel_width = 340;
        const static int panel_height = 50;
        const static int default_log_h = 80;
    };

    class dq_viewer_model : public rs2::viewer_model
    {
    public:
        std::function<void(bool)>           on_left_mouse = [](bool) {};
        std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
        std::function<void(double, double)> on_mouse_move = [](double, double) {};
        std::function<void(int)>            on_key_release = [](int) {};

        dq_viewer_model(int width, int height, const char* title)
            : _win(nullptr), _width(width), _height(height), _output_height(0),
            _font_14(nullptr), _font_18(nullptr), error_message("")
        {
            glfwInit();
            _win = glfwCreateWindow(width, height, title, nullptr, nullptr);
            glfwMakeContextCurrent(_win);

            ImGui_ImplGlfw_Init(_win, true);

            // Load fonts to be used with the ImGui - TODO RAII
            imgui_easy_theming(_font_14, _font_18);

            // Register for UI-controller events
            glfwSetWindowUserPointer(_win, this);
            glfwSetMouseButtonCallback(_win, [](GLFWwindow * win, int button, int action, int mods)
            {
                auto model = (dq_viewer_model*)glfwGetWindowUserPointer(win);
                if (button == 0) model->on_left_mouse(action == GLFW_PRESS);
            });

            glfwSetScrollCallback(_win, [](GLFWwindow * win, double xoffset, double yoffset)
            {
                auto model = (dq_viewer_model*)glfwGetWindowUserPointer(win);
                model->on_mouse_scroll(xoffset, yoffset);
            });

            glfwSetCursorPosCallback(_win, [](GLFWwindow * win, double x, double y)
            {
                auto model = (dq_viewer_model*)glfwGetWindowUserPointer(win);
                model->on_mouse_move(x, y);
                model->_mouse.cursor = { float(x), float(y) };
                //model->_mouse->cursor = { (float)cx / data->scale_factor, (float)cy / data->scale_factor };
            });

            glfwSetKeyCallback(_win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
            {
                auto model = (dq_viewer_model*)glfwGetWindowUserPointer(win);
                if (0 == action) // on key release
                {
                    model->on_key_release(key);
                }
            });
        }

        float width() const { return float(_width); }
        float height() const { return float(_height); }

        // Check that the graphic subsystem is valid and start a new frame
        operator bool()
        {
            auto res = !glfwWindowShouldClose(_win);

            try
            {
                // reset graphic pipe
                commence_frame();

                // Render Depth and ROI
                render_viewports();

                finalize_frame();
            }
            catch (const rs2::error& e)
            {
                error_message = error_to_string(e);
            }
            catch (const std::exception& e)
            {
                error_message = e.what();
            }
            // Render calculated metrics
            return res;
        }

        ~dq_viewer_model()
        {
            ImGui::GetIO().Fonts->ClearFonts();  // To be refactored into Viewer theme object
            ImGui_ImplGlfw_Shutdown();
            glfwDestroyWindow(_win);
            glfwTerminate();
        }

        operator GLFWwindow*() { return _win; }

    private:

        void commence_frame()
        {
            glfwPollEvents();
            glfwGetFramebufferSize(_win, &_width, &_height);

            // Clear the framebuffer
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glViewport(0, 0, _width, _height);

            // Preset OpenGL to 2D rendering
            glPushMatrix();
            glfwGetWindowSize(_win, &_width, &_height);
            glOrtho(0, _width, _height, 0, -1, +1);

            // Reset ImGui state
            ImGui_ImplGlfw_NewFrame(1);

            // Recalculate UI traits
            _output_height = (is_output_collapsed ? viewer_ui_traits::default_log_h : 20);

            _viewer_rect = { (float)viewer_ui_traits::panel_width,
                (float)viewer_ui_traits::panel_height,
                (float)(_width - viewer_ui_traits::panel_width),
                (float)(_height - viewer_ui_traits::panel_height - _output_height) };
        }

        void render_viewports()
        {
            // Rendering
            glViewport(0, 0,
                static_cast<int>(ImGui::GetIO().DisplaySize.x),
                static_cast<int>(ImGui::GetIO().DisplaySize.y));
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            render_2d_view(_viewer_rect, _width, _height, _output_height, _font_14, _font_18,
             1,  // TODO obtain the number of active device models
            this->_mouse, error_message);
        }

        void render_device_panel()
        {
        }

        void render_metrics_panel()
        {
        }

        void finalize_frame()
        {
            ImGui::Render();

            glfwSwapBuffers(_win);
            _mouse.mouse_wheel = 0;
        }

        GLFWwindow* _win;
        int _width, _height, _output_height;
        rs2::rect  _viewer_rect;

        ImFont *_font_14, *_font_18;
        rs2::mouse_info  _mouse;
        std::string error_message;

    };
}