#include <librealsense/rs.hpp>
//#include "example.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"


#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include "concurrency.hpp"
#include <atomic>
#include <map>
#include <sstream>


#pragma comment(lib, "opengl32.lib")

struct option_metadata
{
    rs::option_range range;
    bool supported = false;
    float value = 0.0f;
    const char* label = "";

    bool is_checkbox() const
    {
        return range.max == 1.0f &&
            range.min == 0.0f &&
            range.step == 1.0f;
    }
};

std::map<rs_subdevice, std::map<rs_option, option_metadata>> init_device_options(rs::device& dev)
{
    std::map<rs_subdevice, std::map<rs_option, option_metadata>> options_metadata;
    for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
    {
        auto subdevice = static_cast<rs_subdevice>(j);
        auto&& endpoint = dev.get_subdevice(subdevice);

        for (auto i = 0; i < RS_OPTION_COUNT; i++)
        {
            try
            {
                option_metadata metadata;
                auto opt = static_cast<rs_option>(i);
                metadata.label = rs_option_to_string(opt);
                metadata.supported = endpoint.supports(opt);
                if (metadata.supported)
                {
                    metadata.range = endpoint.get_option_range(opt);
                    metadata.value = endpoint.get_option(opt);
                }
                options_metadata[subdevice][opt] = metadata;
            }
            catch (...) {}
        }
    }
    return options_metadata;
}

int main(int, char**) try
{
    if (!glfwInit())
        exit(1);

    auto window = glfwCreateWindow(1280, 720, "config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(0, 0, 0);

    rs::context ctx;
    auto device_index = 0;
    auto list = ctx.query_devices();
    auto dev = ctx.create(list[device_index]);
    std::vector<std::string> device_names;
    for (auto&& l : list)
    {
        auto d = ctx.create(l);
        auto name = d.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME);
        auto serial = d.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER);
        std::stringstream ss;
        ss << name << " Sn#" << serial;
        device_names.push_back(ss.str());
    }

    std::map<rs_subdevice, std::map<rs_option, option_metadata>> options_metadata;
    for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
    {
        auto subdevice = static_cast<rs_subdevice>(j);
        auto&& endpoint = dev.get_subdevice(subdevice);

        for (auto i = 0; i < RS_OPTION_COUNT; i++)
        {
            try
            {
                option_metadata metadata;
                auto opt = static_cast<rs_option>(i);
                metadata.label = rs_option_to_string(opt);
                metadata.supported = endpoint.supports(opt);
                if (metadata.supported)
                {
                    metadata.range = endpoint.get_option_range(opt);
                    metadata.value = endpoint.get_option(opt);
                }
                options_metadata[subdevice][opt] = metadata;
            }
            catch (...) {}
        }
    }


    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();

        auto y = 0;

        ImGui::SetNextWindowPos({ 700, (float)y });
        ImGui::Begin("Device Selection");
        std::vector<const char*> device_names_chars;
        for (auto&& name : device_names)
        {
            device_names_chars.push_back(name.c_str());
        }
        if (ImGui::Combo("", &device_index, device_names_chars.data(), device_names.size()))
        {
            dev = ctx.create(list[device_index]);
        }

        for (auto i = 0; i < RS_CAMERA_INFO_COUNT; i++)
        {
            auto info = static_cast<rs_camera_info>(i);
            if (dev.supports(info))
            {
                std::stringstream ss;
                ss << rs_camera_info_to_string(info) << ": " << dev.get_camera_info(info);
                auto line = ss.str();
                ImGui::Text(line.c_str());
            }
        }

        y += ImGui::GetWindowSize().y;
        ImGui::End();

        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            auto&& endpoint = dev.get_subdevice(subdevice);

            ImGui::SetNextWindowPos({ 700, (float)y });
            ImGui::Begin(rs_subdevice_to_string(subdevice));
            for (auto i = 0; i < RS_OPTION_COUNT; i++)
            {
                auto opt = static_cast<rs_option>(i);
                auto&& metadata = options_metadata[subdevice][opt];

                if (metadata.supported)
                {
                    if (metadata.is_checkbox())
                    {
                        auto value = metadata.value > 0.0f;
                        if (ImGui::Checkbox(metadata.label, &value))
                        {
                            metadata.value = value ? 1.0f : 0.0f;
                            endpoint.set_option(opt, metadata.value);
                        }
                    }
                    else
                    {
                        ImGui::Text(metadata.label);
                        if (ImGui::SliderFloat("", &metadata.value,
                            metadata.range.min, metadata.range.max))
                        {
                            // TODO: Round to step?
                            endpoint.set_option(opt, metadata.value);
                        }
                    }
                }
            }
            
            y += ImGui::GetWindowSize().y;
            ImGui::End();
        }

        // Rendering
        glViewport(0, 0, 
            static_cast<int>(ImGui::GetIO().DisplaySize.x), 
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
