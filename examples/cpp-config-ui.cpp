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


#pragma comment(lib, "opengl32.lib")


int main(int, char**) try
{
    if (!glfwInit())
        exit(1);

    auto window = glfwCreateWindow(1280, 720, "config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(0, 0, 0);

    rs::context ctx;
    auto list = ctx.query_devices();
    auto dev = ctx.create(list[0]);

    struct option_metadata
    {
        rs::option_range range;
        bool supported;
        float value;
        const char* label;
    };

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

        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            auto&& endpoint = dev.get_subdevice(subdevice);

            ImGui::Begin(rs_subdevice_to_string(subdevice));
            for (auto i = 0; i < RS_OPTION_COUNT; i++)
            {
                auto opt = static_cast<rs_option>(i);
                auto&& metadata = options_metadata[subdevice][opt];

                if (metadata.supported)
                {
                    if (ImGui::SliderFloat(metadata.label, &metadata.value, 
                        metadata.range.min, metadata.range.max))
                    {
                        // TODO: Round to step?
                        dev.color().set_option(opt, metadata.value);
                    }
                }
            }
            
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
