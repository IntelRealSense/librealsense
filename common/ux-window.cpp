#include "ux-window.h"

#include "model-views.h"

// We use STB image to load the splash-screen from memory
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// int-rs-splash.hpp contains the PNG image from res/int-rs-splash.png
#include "res/int-rs-splash.hpp"

namespace rs2
{
    ux_window::ux_window(const char* title) :
        _win(nullptr), _width(0), _height(0), _output_height(0),
        _font_14(nullptr), _font_18(nullptr), _app_ready(false)
    {
        if (!glfwInit()) exit(1);

        rs2_error* e = nullptr;
        _title_str = to_string() << title << " v" << api_version_to_string(rs2_get_api_version(&e));

        auto primary = glfwGetPrimaryMonitor();
        const auto mode = glfwGetVideoMode(primary);

        // Create GUI Windows
        _width = int(mode->width * 0.7f);
        _height = int(mode->height * 0.7f);
        _win = glfwCreateWindow(_width, _height, _title_str.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(_win);
        ImGui_ImplGlfw_Init(_win, true);

        // Load fonts to be used with the ImGui - TODO move to RAII
        imgui_easy_theming(_font_14, _font_18);

        // Register for UI-controller events
        glfwSetWindowUserPointer(_win, this);


        glfwSetCursorPosCallback(_win, [](GLFWwindow* w, double cx, double cy)
        {
            auto data = reinterpret_cast<ux_window*>(glfwGetWindowUserPointer(w));
            data->_mouse.cursor = { (float)cx / data->_scale_factor,
                (float)cy / data->_scale_factor };
        });
        glfwSetMouseButtonCallback(_win, [](GLFWwindow* w, int button, int action, int mods)
        {
            auto data = reinterpret_cast<ux_window*>(glfwGetWindowUserPointer(w));
            data->_mouse.mouse_down = (button == GLFW_MOUSE_BUTTON_1) && (action != GLFW_RELEASE);
        });
        glfwSetScrollCallback(_win, [](GLFWwindow * w, double xoffset, double yoffset)
        {
            auto data = reinterpret_cast<ux_window*>(glfwGetWindowUserPointer(w));
            data->_mouse.mouse_wheel = yoffset;
            data->_mouse.ui_wheel += yoffset;
        });

        glfwSetDropCallback(_win, [](GLFWwindow* w, int count, const char** paths)
        {
            auto data = reinterpret_cast<ux_window*>(glfwGetWindowUserPointer(w));

            if (count <= 0) return;

            for (int i = 0; i < count; i++)
            {
                data->on_file_drop(paths[i]);
            }
        });

        // Prepare the splash screen and do some initialization in the background
        int x, y, comp;
        auto r = stbi_load_from_memory(splash, splash_size, &x, &y, &comp, false);
        _splash_tex.upload_image(x, y, r);

    }

    // Check that the graphic subsystem is valid and start a new frame
    ux_window::operator bool()
    {
        end_frame();

        // Yield the CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto res = !glfwWindowShouldClose(_win);

        if (_first_frame)
        {
            std::thread first_load([&]() {
                on_load();
                _app_ready = true;
            });
            first_load.detach();

            _first_frame = false;
        }

        // If we are just getting started, render the Splash Screen instead of normal UI
        while (res && (!_app_ready || _splash_timer.elapsed_ms() < 1500))
        {
            res = !glfwWindowShouldClose(_win);
            glfwPollEvents();

            begin_frame();

            glPushMatrix();
            glViewport(0.f, 0.f, _width, _height);
            glClearColor(0.036f, 0.044f, 0.051f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            glLoadIdentity();
            glOrtho(0, _width, _height, 0, -1, +1);

            // Fade-in the logo
            auto opacity = smoothstep(_splash_timer.elapsed_ms(), 100.f, 2000.f);
            _splash_tex.show({ 0.f,0.f,(float)_width,(float)_height }, opacity);

            static bool missing_device = false;
            static int hourglass_index = 0;
            std::string hourglass = u8"\uf250";
            static periodic_timer every_200ms(std::chrono::milliseconds(200));
            if (every_200ms)
            {
                missing_device = rs2::context().query_devices().size() == 0;
                hourglass_index = (hourglass_index + 1) % 5;
            }
            hourglass[2] += hourglass_index;

            bool blink = sin(_splash_timer.elapsed_ms() / 150.f) > -0.3f;

            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
            ImGui::SetNextWindowPos({ (float)_width/2 - 150, (float)_height/2 + 70 });
            ImGui::PushFont(_font_18);
            ImGui::SetNextWindowSize({ (float)_width, (float)_height });
            ImGui::Begin("Splash Screen Banner", nullptr, flags);

            if (missing_device || blink)
            {
                ImGui::Text("%s   Loading %s...", hourglass.c_str(), _title_str.c_str());
            }
            if (missing_device && blink)
            {
                ImGui::Text(u8"\uf287 Please connect Intel RealSense device!");
            }
            ImGui::End();
            ImGui::PopFont();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);

            end_frame();

            //glfwSwapBuffers(_win);
            glPopMatrix();

            // Yield the CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // reset graphic pipe
        begin_frame();

        return res;
    }

    ux_window::~ux_window()
    {
        ImGui::GetIO().Fonts->ClearFonts();  // To be refactored into Viewer theme object
        ImGui_ImplGlfw_Shutdown();
        glfwDestroyWindow(_win);
        glfwTerminate();
    }

    void ux_window::begin_frame()
    {
        glfwPollEvents();
        glfwGetFramebufferSize(_win, &_width, &_height);

        // Update the scale factor each frame
        // based on resolution and physical display size
        _scale_factor = pick_scale_factor(_win);
        _width = _width / _scale_factor;
        _height = _height / _scale_factor;

        // Reset ImGui state
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        ImGui::GetIO().MouseWheel = _mouse.ui_wheel;
        _mouse.ui_wheel = 0.f;

        ImGui_ImplGlfw_NewFrame(_scale_factor);
    }

    void ux_window::begin_viewport()
    {
        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x * _scale_factor),
            static_cast<int>(ImGui::GetIO().DisplaySize.y * _scale_factor));
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void ux_window::end_frame()
    {
        if (!_first_frame)
        {
            ImGui::Render();

            glfwSwapBuffers(_win);
            _mouse.mouse_wheel = 0;
        }
    }
}
