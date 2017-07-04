// See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "../include/rs4xx_advanced_mode.hpp"
#include <librealsense/rs2.hpp>
#include <regex>

#include "sample.hpp"
#include "../src/concurrency.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <sstream>
#include <thread>
#include <iostream>
#include <memory>

#include "tclap/CmdLine.h"

#include "../src/json_loader.hpp"

using namespace TCLAP;

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

std::string error_to_string(const rs2::error& e)
{
    return to_string() << e.get_failed_function() << "("
        << e.get_failed_args() << "):\n" << e.what();
}

bool no_device_popup(GLFWwindow* window, const ImVec4& clear_color)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int w, h;
        glfwGetWindowSize(window, &w, &h);

        ImGui_ImplGlfw_NewFrame();

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ static_cast<float>(w), static_cast<float>(h) });
        ImGui::Begin("", nullptr, flags);

        ImGui::OpenPopup("R400 Advanced Mode");
        if (ImGui::BeginPopupModal("R400 Advanced Mode", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("No device detected. Is it plugged in?");
            ImGui::Separator();

            if (ImGui::Button("Retry", ImVec2(120, 0)))
            {
                return true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit", ImVec2(120, 0)))
            {
                return false;
            }

            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::Render();
        glfwSwapBuffers(window);
    }
    return false;
}

bool device_connected(const rs2::context& ctx)
{
    auto list = ctx.query_devices();
    return list.size() > 0;
}

std::shared_ptr<rs2::device> get_device(rs2::context& ctx)
{
    auto list = ctx.query_devices();
    return std::make_shared<rs2::device>(list[0]);
}

template<class T, class S>
void checkbox(const char* id, T* val, S T::* f, bool& to_set)
{
    bool temp = (val->*f) > 0;

    if (ImGui::Checkbox(id, &temp))
    {
        val->*f = temp ? 1 : 0;
        to_set = true;
    }
}

template<class T, class S>
void slider_int(const char* id, T* val, S T::* feild, bool& to_set)
{
    ImGui::Text("%s", id);
    int temp = val->*feild;
    int min = (val + 1)->*feild;
    int max = (val + 2)->*feild;

    if (ImGui::SliderInt(id, &temp, min, max))
    {
        val->*feild = temp;
        to_set = true;
    }
}

template<class T, class S>
void slider_float(const char* id, T* val, S T::* feild, bool& to_set)
{
    ImGui::Text("%s", id);
    float temp = val->*feild;
    float min = (val + 1)->*feild;
    float max = (val + 2)->*feild;

    if (ImGui::SliderFloat(id, &temp, min, max))
    {
        val->*feild = temp;
        to_set = true;
    }
}

template<class T>
void write_advanced(const rs4xx::advanced_mode& advanced, const T& t, bool to_set, std::string& error_message)
{
    if (to_set)
    {
        try {
            advanced.set(t.vals[0]);
        }
        catch (const rs2::error & e)
        {
            error_message = error_to_string(e);
        }
        catch (const std::exception & e)
        {
            error_message = e.what();
        }
    }
}

auto restart = true;
std::string filename = "";

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    for (auto i = 0; i < count; i++)
    {
        filename = paths[i];
        restart = true;
    }
}

rs2_format parse(const std::string& format)
{
    for (int i = RS2_FORMAT_ANY + 1; i < RS2_FORMAT_COUNT; i++)
    {
        if (format == rs2_format_to_string((rs2_format)i))
        {
            return (rs2_format)i;
        }
    }
    if (format == "COUNT") return RS2_FORMAT_COUNT;
    throw std::runtime_error("Stream format \"" + format + "\" not found!");
}

float parse_option(const rs2::device& dev, rs2_option opt, const std::string& val)
{
    auto range = dev.get_option_range(opt);
    if (range.step == 0) return range.min;
    for (auto i = range.min; i <= range.max; i += range.step)
    {
        if (dev.get_option_value_description(opt, i) == val)
            return i;
    }
    return range.min;
}

int main(int argc, char** argv) try
{
    auto preset = -1;
    auto started = false;
    std::string error_message = "";

    std::vector<const char*> json_files;
    std::vector<std::string> preset_files;

    try
    {
        preset_files = enumerate_dir("./presets");

        std::regex is_json(".*\\.json");
        for (auto& file : preset_files)
        {
            if (regex_match(file, is_json))
            {
                json_files.push_back(file.c_str());
            }
        }
    }
    catch (...)
    {
        error_message = "Presets folder not found in running directory!";
    }

    CmdLine cmd("Advanced Mode Sample App", ' ', RS2_API_VERSION_STR);

    ValueArg<std::string> filename_arg("f", "filename", "Filename of the JSON file", false, "", "string");
    cmd.add(filename_arg);

    cmd.parse(argc, argv);

    filename = filename_arg.getValue();

    if (!glfwInit())
        exit(1);

    auto window = glfwCreateWindow(1280, 720, "R400 Advanced Mode Sample", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    glfwSetDropCallback(window, drop_callback);

    ImVec4 clear_color = ImColor(10, 0, 0);

    rs2::context ctx;

    while (!device_connected(ctx))
    {
        if (!no_device_popup(window, clear_color)) return EXIT_SUCCESS;
    }

    auto dev = get_device(ctx);

    single_consumer_queue<rs2::frame> queue;
    texture_buffer tex_buffer[RS2_STREAM_COUNT];

    auto send_receive = [&dev](const std::vector<uint8_t>& input)
    {
        return dev->debug().send_and_receive_raw_data(input);
    };

    rs4xx::advanced_mode advanced(send_receive);

    camera_state state;
    // Work-around, HDAD feild deos not contain MIN/MAX:
    state.hdad.vals[1].ignoreSAD = 0;       state.hdad.vals[2].ignoreSAD = 1;
    state.hdad.vals[1].lambdaAD = 0;        state.hdad.vals[2].lambdaAD = 1000000;
    state.hdad.vals[1].lambdaCensus = 0;    state.hdad.vals[2].lambdaCensus = 1000000;

    state.ae.vals[1].meanIntensitySetPoint = 0; state.ae.vals[2].meanIntensitySetPoint = 1000000;

    state.profile.vals[1].width = 100; state.profile.vals[2].width = 10000;
    state.profile.vals[1].height = 100; state.profile.vals[2].height = 10000;
    state.profile.vals[1].fps = 10; state.profile.vals[2].fps = 180;

    auto exposure_range = dev->get_option_range(RS2_OPTION_EXPOSURE);
    state.exposure.vals[1].exposure = exposure_range.min; state.exposure.vals[2].exposure = exposure_range.max;
    auto ae_range = dev->get_option_range(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
    state.exposure.vals[1].auto_exposure = ae_range.min; state.exposure.vals[2].auto_exposure = ae_range.max;

    auto laser_range = dev->get_option_range(RS2_OPTION_LASER_POWER);
    state.laser.vals[1].laser_power = laser_range.min; state.laser.vals[2].laser_power = laser_range.max;
    auto laser_mode_range = dev->get_option_range(RS2_OPTION_EMITTER_ENABLED);
    state.laser.vals[1].laser_state = laser_mode_range.min; state.laser.vals[2].laser_state = laser_mode_range.max;

    state.roi.vals[1].roi_top = 0; state.roi.vals[2].roi_top = 0xFFFF;
    state.roi.vals[1].roi_left = 0; state.roi.vals[2].roi_left = 0xFFFF;
    state.roi.vals[1].roi_right = 0; state.roi.vals[2].roi_right = 0xFFFF;
    state.roi.vals[1].roi_bottom = 0; state.roi.vals[2].roi_bottom = 0xFFFF;

    auto callback = [&](rs2::frame f)
    {
        if (queue.size() < 2) queue.enqueue(std::move(f));
    };

    try
    {
        if (!advanced.is_enabled())
        {
            // Go to advanced mode and re-create the device
            advanced.go_to_advanced_mode();
            std::this_thread::sleep_for(std::chrono::seconds(1));

            while (!device_connected(ctx))
            {
                if (!no_device_popup(window, clear_color)) return EXIT_SUCCESS;
            }

            dev = get_device(ctx);
        }
    }
    catch (const rs2::error & e)
    {
        error_message = error_to_string(e);
    }

    auto width = 640;
    auto height = 480;
    auto fps = 30;
    auto depth_format = RS2_FORMAT_Z16;
    auto ir_format = RS2_FORMAT_COUNT;
    auto selected_view = 0;
    std::vector<const char*> views = { "Depth", "Infrared" };
    std::map<int, rs2_stream> view_to_format { { 0, RS2_STREAM_DEPTH }, { 1, RS2_STREAM_INFRARED } };

    while (!glfwWindowShouldClose(window))
    {
        try
        {
            if (restart)
            {

                for (auto k = 0; k < 3; k++)
                {
                    // Get Current Algo Control Values
                    advanced.get(state.depth_controls.vals + k, k);
                    advanced.get(state.rsm.vals + k, k);
                    advanced.get(state.rsvc.vals + k, k);
                    advanced.get(state.color_control.vals + k, k);
                    advanced.get(state.rctc.vals + k, k);
                    advanced.get(state.sctc.vals + k, k);
                    advanced.get(state.spc.vals + k, k);
                    advanced.get(state.cc.vals + k, k);
                    advanced.get(state.depth_table.vals + k, k);
                    advanced.get(state.census.vals + k, k);
                }
                advanced.get(state.hdad.vals);
                advanced.get(state.ae.vals);

                state.reset();

                if (started)
                {
                    dev->stop();
                    dev->close();
                    started = false;
                }

                if (filename != "")
                {
                    try
                    {
                        load_json(filename, state);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }

                    if (error_message == "")
                    {
                        std::stringstream errors;

                        if (state.depth_controls.update)
                            try
                            {
                                advanced.set(*state.depth_controls.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Depth Control: " << e.what() << "\n";
                            }

                        if (state.rsm.update)
                            try
                            {
                                advanced.set(*state.rsm.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Rsm: " << e.what() << "\n";
                            }

                        if (state.rsvc.update)
                            try
                            {
                                advanced.set(*state.rsvc.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Rau Support Vector Control: " << e.what() << "\n";
                            }

                        if (state.color_control.update)
                            try
                            {
                                advanced.set(*state.color_control.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Color Control: " << e.what() << "\n";
                            }

                        if (state.rctc.update)
                            try
                            {
                                advanced.set(*state.rctc.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Rau Color Thresholds Control: " << e.what() << "\n";
                            }

                        if (state.sctc.update)
                            try
                            {
                                advanced.set(*state.sctc.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set SLO Color Thresholds Control: " << e.what() << "\n";
                            }

                        if (state.spc.update)
                            try
                            {
                                advanced.set(*state.spc.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set SLO Penalty Control: " << e.what() << "\n";
                            }

                        if (state.hdad.update)
                            try
                            {
                                advanced.set(*state.hdad.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set HDAD: " << e.what() << "\n";
                            }

                        if (state.cc.update)
                            try
                            {
                                advanced.set(*state.cc.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Color Correction: " << e.what() << "\n";
                            }

                        if (state.depth_table.update)
                            try
                            {
                                advanced.set(*state.depth_table.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Depth Table: " << e.what() << "\n";
                            }

                       if (state.ae.update)
                            try
                            {
                                advanced.set(*state.ae.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Auto-Exposure Params: " << e.what() << "\n";
                            }

                        if (state.census.update)
                            try
                            {
                                advanced.set(*state.census.vals);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Census Control: " << e.what() << "\n";
                            }

                        if (state.profile.update)
                            try
                            {
                                depth_format = parse(state.profile.vals[0].depth_format);
                                ir_format = parse(state.profile.vals[0].ir_format);
                                width = state.profile.vals[0].width;
                                height = state.profile.vals[0].height;
                                fps = state.profile.vals[0].fps;
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Stream Profile: " << e.what() << "\n";
                            }

                        if (state.laser.update)
                            try
                            {
                                auto val = parse_option(*dev, RS2_OPTION_EMITTER_ENABLED, state.laser.vals[0].laser_state);
                                dev->set_option(RS2_OPTION_EMITTER_ENABLED, val);
                                if (state.laser.vals[0].laser_state == "On")
                                {
                                    dev->set_option(RS2_OPTION_LASER_POWER, state.laser.vals[0].laser_power);
                                }
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Laser Control: " << e.what() << "\n";
                            }

                        if (state.exposure.update)
                            try
                            {
                                auto val = (state.exposure.vals[0].auto_exposure == "true") ? 1.f : 0.f;

                                dev->set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, val);
                                dev->set_option(RS2_OPTION_EXPOSURE, state.exposure.vals[0].exposure);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Exposure Control: " << e.what() << "\n";
                            }

                        if (state.roi.update)
                            try
                            {
                                rs2::region_of_interest roi
                                {
                                    state.roi.vals[0].roi_left,
                                    state.roi.vals[0].roi_top,
                                    state.roi.vals[0].roi_right,
                                    state.roi.vals[0].roi_bottom
                                };

                                dev->set_region_of_interest(roi);
                            }
                            catch (const std::exception& e)
                            {
                                errors << "Could not set Region of Interest: " << e.what() << "\n";
                            }

                        error_message = errors.str();
                    }

                    filename = "";
                }

                std::vector<rs2::stream_profile> profiles;
                if (depth_format != RS2_FORMAT_COUNT)
                    profiles.push_back({ RS2_STREAM_DEPTH, width, height, fps, depth_format });
                if (ir_format != RS2_FORMAT_COUNT)
                    profiles.push_back({ RS2_STREAM_INFRARED, width, height, fps, ir_format });

                dev->open(profiles);
                dev->start(callback);

                started = true;

                restart = false;
            }
        }
        catch (const rs2::error& e)
        {
            error_message = error_to_string(e);
            restart = false;

            // Backtrack to known configuration
            width = 640;
            height = 480;
            fps = 30;
            depth_format = RS2_FORMAT_Z16;
            ir_format = RS2_FORMAT_COUNT;
        }

        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        ImGui_ImplGlfw_NewFrame();

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 300, static_cast<float>(h) });
        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("librealsense version: ");
        ImGui::SameLine();
        ImGui::Text(RS2_API_VERSION_STR);

        ImGui::Text("JSON presets:");
        ImGui::PushItemWidth(-1);
        if (ImGui::Combo("JSON Preset", &preset, json_files.data(), json_files.size()))
        {
            filename = json_files[preset];
            restart = true;
        }
        ImGui::PopItemWidth();
        ImGui::Text("(you can also pass JSON filename");
        ImGui::Text(" using -f command line switch,");
        ImGui::Text(" or just drag&drop JSONs here)");

        if (ImGui::CollapsingHeader("Stream Profile", nullptr, true, true))
        {
            ImGui::Text("Depth Format: "); ImGui::SameLine(); ImGui::Text("%s", rs2_format_to_string(depth_format));
            ImGui::Text("IR Format: "); ImGui::SameLine(); ImGui::Text("%s", rs2_format_to_string(ir_format));
            std::stringstream ss;
            ss << "Resolution: " << width << " x " << height << " px";
            ImGui::Text("%s", ss.str().c_str()); ss.str("");
            ss << "FPS: " << fps << " Hz";
            ImGui::Text("%s", ss.str().c_str()); ss.str("");

            ImGui::Text("Current View:");
            ImGui::PushItemWidth(-1);
            ImGui::Combo("Selected View", &selected_view, views.data(), views.size());
            ImGui::PopItemWidth();
        }

        if (ImGui::CollapsingHeader("Depth Control", nullptr, true, true))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_int("DS Second Peak Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::deepSeaSecondPeakThreshold, to_set);
            slider_int("DS Neighbor Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::deepSeaNeighborThreshold, to_set);
            slider_int("DS Median Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::deepSeaMedianThreshold, to_set);
            slider_int("Estimate Median Increment", state.depth_controls.vals, &rs4xx::STDepthControlGroup::plusIncrement, to_set);
            slider_int("Estimate Median Decrement", state.depth_controls.vals, &rs4xx::STDepthControlGroup::minusDecrement, to_set);
            slider_int("Score Minimum Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::scoreThreshA, to_set);
            slider_int("Score Maximum Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::scoreThreshB, to_set);
            slider_int("DS LR Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::lrAgreeThreshold, to_set);
            slider_int("Texture Count Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::textureCountThreshold, to_set);
            slider_int("Texture Difference Threshold", state.depth_controls.vals, &rs4xx::STDepthControlGroup::textureDifferenceThreshold, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.depth_controls, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Rsm", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            checkbox("RSM Bypass", state.rsm.vals, &rs4xx::STRsm::rsmBypass, to_set);
            slider_float("Disparity Difference Threshold", state.rsm.vals, &rs4xx::STRsm::diffThresh, to_set);
            slider_float("SLO RAU Difference Threshold", state.rsm.vals, &rs4xx::STRsm::sloRauDiffThresh, to_set);
            slider_int("Remove Threshold", state.rsm.vals, &rs4xx::STRsm::removeThresh, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.rsm, to_set, error_message);
        }


        if (ImGui::CollapsingHeader("Rau Support Vector Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_int("Min West", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minWest, to_set);
            slider_int("Min East", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minEast, to_set);
            slider_int("Min WE Sum", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minWEsum, to_set);
            slider_int("Min North", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minNorth, to_set);
            slider_int("Min South", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minSouth, to_set);
            slider_int("Min NS Sum", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::minNSsum, to_set);
            slider_int("U Shrink", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::uShrink, to_set);
            slider_int("V Shrink", state.rsvc.vals, &rs4xx::STRauSupportVectorControl::vShrink, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.rsvc, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Color Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            checkbox("Disable SAD Color", state.color_control.vals, &rs4xx::STColorControl::disableSADColor, to_set);
            checkbox("Disable RAU Color", state.color_control.vals, &rs4xx::STColorControl::disableRAUColor, to_set);
            checkbox("Disable SLO Right Color", state.color_control.vals, &rs4xx::STColorControl::disableSLORightColor, to_set);
            checkbox("Disable SLO Left Color", state.color_control.vals, &rs4xx::STColorControl::disableSLOLeftColor, to_set);
            checkbox("Disable SAD Normalize", state.color_control.vals, &rs4xx::STColorControl::disableSADNormalize, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.color_control, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Rau Color Thresholds Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_int("Diff Threshold Red", state.rctc.vals, &rs4xx::STRauColorThresholdsControl::rauDiffThresholdRed, to_set);
            slider_int("Diff Threshold Green", state.rctc.vals, &rs4xx::STRauColorThresholdsControl::rauDiffThresholdGreen, to_set);
            slider_int("Diff Threshold Blue", state.rctc.vals, &rs4xx::STRauColorThresholdsControl::rauDiffThresholdBlue, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.rctc, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("SLO Color Thresholds Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_int("Diff Threshold Red", state.sctc.vals, &rs4xx::STSloColorThresholdsControl::diffThresholdRed, to_set);
            slider_int("Diff Threshold Green", state.sctc.vals, &rs4xx::STSloColorThresholdsControl::diffThresholdGreen, to_set);
            slider_int("Diff Threshold Blue", state.sctc.vals, &rs4xx::STSloColorThresholdsControl::diffThresholdBlue, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.sctc, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("SLO Penalty Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_int("K1 Penalty", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK1Penalty, to_set);
            slider_int("K2 Penalty", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK2Penalty, to_set);
            slider_int("K1 Penalty Mod1", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK1PenaltyMod1, to_set);
            slider_int("K1 Penalty Mod2", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK1PenaltyMod2, to_set);
            slider_int("K2 Penalty Mod1", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK2PenaltyMod1, to_set);
            slider_int("K2 Penalty Mod2", state.spc.vals, &rs4xx::STSloPenaltyControl::sloK2PenaltyMod2, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.spc, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("HDAD", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            checkbox("Ignore SAD", state.hdad.vals, &rs4xx::STHdad::ignoreSAD, to_set);

            // TODO: Not clear from documents what is the valid range:
            slider_float("AD Lambda", state.hdad.vals, &rs4xx::STHdad::lambdaAD, to_set);
            slider_float("Census Lambda", state.hdad.vals, &rs4xx::STHdad::lambdaCensus, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.hdad, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Color Correction", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_float("Color Correction 1", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection1,  to_set);
            slider_float("Color Correction 2", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection2,  to_set);
            slider_float("Color Correction 3", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection3,  to_set);
            slider_float("Color Correction 4", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection4,  to_set);
            slider_float("Color Correction 5", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection5,  to_set);
            slider_float("Color Correction 6", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection6,  to_set);
            slider_float("Color Correction 7", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection7,  to_set);
            slider_float("Color Correction 8", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection8,  to_set);
            slider_float("Color Correction 9", state.cc.vals, &rs4xx::STColorCorrection::colorCorrection9,  to_set);
            slider_float("Color Correction 10",state.cc.vals, &rs4xx::STColorCorrection::colorCorrection10, to_set);
            slider_float("Color Correction 11",state.cc.vals, &rs4xx::STColorCorrection::colorCorrection11, to_set);
            slider_float("Color Correction 12",state.cc.vals, &rs4xx::STColorCorrection::colorCorrection12, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.cc, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Depth Table", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_float("Depth Units", state.depth_table.vals, &rs4xx::STDepthTableControl::depthUnits, to_set);
            slider_float("Depth Clamp Min", state.depth_table.vals, &rs4xx::STDepthTableControl::depthClampMin, to_set);
            slider_float("Depth Clamp Max", state.depth_table.vals, &rs4xx::STDepthTableControl::depthClampMax, to_set);
            slider_float("Disparity Mode", state.depth_table.vals, &rs4xx::STDepthTableControl::disparityMode, to_set);
            slider_float("Disparity Shift", state.depth_table.vals, &rs4xx::STDepthTableControl::disparityShift, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.depth_controls, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("AE Control", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_float("Mean Intensity Set Point", state.ae.vals, &rs4xx::STAEControl::meanIntensitySetPoint, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.ae, to_set, error_message);
        }

        if (ImGui::CollapsingHeader("Census Enable Reg", nullptr, true, false))
        {
            ImGui::PushItemWidth(-1);

            auto to_set = false;

            slider_float("u-Diameter", state.census.vals, &rs4xx::STCensusRadius::uDiameter, to_set);
            slider_float("v-Diameter", state.census.vals, &rs4xx::STCensusRadius::vDiameter, to_set);

            ImGui::PopItemWidth();

            write_advanced(advanced, state.census, to_set, error_message);
        }

        if (error_message != "")
            ImGui::OpenPopup("Oops, something went wrong!");
        if (ImGui::BeginPopupModal("Oops, something went wrong!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("RealSense error calling:");
            ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
                error_message.size(), { 700, 150 }, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                error_message = "";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        rs2::frame f;
        if (queue.try_dequeue(&f))
        {
            auto stream = f.get_stream_type();
            tex_buffer[stream].upload(f);
        }

        glfwGetWindowSize(window, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        tex_buffer[view_to_format[selected_view]].show({ 300.0f, 0.0f, (float)w - 300.0f, (float)h });

        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
