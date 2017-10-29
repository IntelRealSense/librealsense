// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <map>
#include <algorithm>
#include <mutex>
#include <vector>

class sample_gui
{
    class option_model
    {
    public:
        bool draw(std::string& error_message)
        {
            auto res = false;
            if (supported)
            {
                auto desc = endpoint.get_option_description(opt);

                if (is_checkbox())
                {
                    auto bool_value = value > 0.0f;
                    if (ImGui::Checkbox(label.c_str(), &bool_value))
                    {
                        res = true;
                        value = bool_value ? 1.0f : 0.0f;
                        try
                        {
                            endpoint.set_option(opt, value);
                        }
                        catch (const rs2::error& e)
                        {
                            //error_message = error_to_string(e);
                        }
                    }
                    if (ImGui::IsItemHovered() && desc)
                    {
                        ImGui::SetTooltip("%s", desc);
                    }
                }
                else
                {
                    if (!is_enum())
                    {
                        std::string txt =std::string(rs2_option_to_string(opt)) + ":";
                        ImGui::Text("%s", txt.c_str());

                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 0.5f, 0.5f, 1.f });
                        ImGui::Text("(?)");
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered() && desc)
                        {
                            ImGui::SetTooltip("%s", desc);
                        }

                        ImGui::PushItemWidth(-1);

                        try
                        {
                            if (read_only)
                            {
                                ImVec2 vec{ 0, 14 };
                                std::string text = (value == (int)value) ? std::to_string((int)value) : std::to_string(value);
                                if (range.min != range.max)
                                {
                                    ImGui::ProgressBar((value / (range.max - range.min)), vec, text.c_str());
                                }
                                else //constant value options
                                {
                                    auto c = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_FrameBg));
                                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, c);
                                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c);
                                    float dummy = (int)value;
                                    ImGui::DragFloat(id.c_str(), &dummy, 1, 0, 0, text.c_str());
                                    ImGui::PopStyleColor(2);
                                }
                            }
                            else if (is_all_integers())
                            {
                                auto int_value = static_cast<int>(value);
                                if (ImGui::SliderIntWithSteps(id.c_str(), &int_value,
                                    static_cast<int>(range.min),
                                    static_cast<int>(range.max),
                                    static_cast<int>(range.step)))
                                {
                                    // TODO: Round to step?
                                    value = static_cast<float>(int_value);
                                    endpoint.set_option(opt, value);
                                    res = true;
                                }
                            }
                            else
                            {
                                if (ImGui::SliderFloat(id.c_str(), &value,
                                    range.min, range.max, "%.4f"))
                                {
                                    endpoint.set_option(opt, value);
                                    res = true;
                                }
                            }
                        }
                        catch (const rs2::error& e)
                        {
                            //error_message = error_to_string(e);
                        }
                    }
                    else
                    {
                        std::string txt = std::string(rs2_option_to_string(opt)) + ":";
                        auto col_id = id + "columns";
                        //ImGui::Columns(2, col_id.c_str(), false);
                        //ImGui::SetColumnOffset(1, 120);



                        ImGui::Text("%s", txt.c_str());
                        if (ImGui::IsItemHovered() && desc)
                        {
                            ImGui::SetTooltip("%s", desc);
                        }

                        ImGui::SameLine(); ImGui::SetCursorPosX(135);

                        ImGui::PushItemWidth(-1);

                        std::vector<const char*> labels;
                        auto selected = 0, counter = 0;
                        for (auto i = range.min; i <= range.max; i += range.step, counter++)
                        {
                            if (std::fabs(i - value) < 0.001f) selected = counter;
                            labels.push_back(endpoint.get_option_value_description(opt, i));
                        }
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });

                        try
                        {
                            if (ImGui::Combo(id.c_str(), &selected, labels.data(),
                                static_cast<int>(labels.size())))
                            {
                                value = range.min + range.step * selected;
                                endpoint.set_option(opt, value);
                                res = true;
                            }
                        }
                        catch (const rs2::error& e)
                        {
                            //error_message = error_to_string(e);
                        }

                        ImGui::PopStyleColor();

                        ImGui::PopItemWidth();

                        ImGui::NextColumn();
                        ImGui::Columns(1);
                    }


                }

                if (!read_only && opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE /*&& dev->auto_exposure_enabled*/)
                {
                    ImGui::SameLine(0, 10);
                    std::string button_label = label;
                    auto index = label.find_last_of('#');
                    if (index != std::string::npos)
                    {
                        button_label = label.substr(index + 1);
                    }

                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1.f,1.f,1.f,1.f });
                    //if (!dev->roi_checked)
                    //{
                    //    std::string caption = "Set ROI##" + button_label;
                    //    if (ImGui::Button(caption.c_str(), { 55, 0 }))
                    //    {
                    //        dev->roi_checked = true;
                    //    }
                    //}
                    //else
                    //{
                    //    std::string caption = "Cancel##" + button_label;
                    //    if (ImGui::Button(caption.c_str(), { 55, 0 }))
                    //    {
                    //        dev->roi_checked = false;
                    //    }
                    //}
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Select custom region of interest for the auto-exposure algorithm\nClick the button, then draw a rect on the frame");
                }
            }

            return res;
        }

        void update_supported(std::string& error_message)
        {
            try
            {
                supported = endpoint.supports(opt);
            }
            catch (const rs2::error& e)
            {
                //error_message = error_to_string(e);
            }
        }

        void update_read_only_status(std::string& error_message)
        {
            try
            {
                read_only = endpoint.is_option_read_only(opt);
            }
            catch (const rs2::error& e)
            {
                //error_message = error_to_string(e);
            }
        }

        void update_all_fields(std::string& error_message)
        {
            try
            {
                if (supported = endpoint.supports(opt))
                {
                    value = endpoint.get_option(opt);
                    range = endpoint.get_option_range(opt);
                    read_only = endpoint.is_option_read_only(opt);
                }
            }
            catch (const rs2::error& e)
            {

            }
        }
        bool is_integer(float f) const
        {
            return (fabs(fmod(f, 1)) < std::numeric_limits<float>::min());
        }
        bool is_all_integers() const
        {
            return is_integer(range.min) && is_integer(range.max) &&
                is_integer(range.def) && is_integer(range.step);
        }

        bool is_enum() const
        {
            if (range.step < 0.001f) return false;

            for (auto i = range.min; i <= range.max; i += range.step)
            {
                if (endpoint.get_option_value_description(opt, i) == nullptr)
                    return false;
            }
            return true;
        }

        bool is_checkbox() const
        {
            return range.max == 1.0f &&
                range.min == 0.0f &&
                range.step == 1.0f;
        }


        rs2_option opt;
        rs2::option_range range;
        rs2::sensor endpoint;
        bool supported = false;
        bool read_only = false;
        float value = 0.0f;
        std::string label = "";
        std::string id = "";

    };
    class latest_frames
    {
        std::map<int, rs2::frame> _latest_frames;
        std::mutex m;
    public:
        void put(const rs2::frame& f)
        {
            std::lock_guard<std::mutex> lock(m);
            _latest_frames[f.get_profile().unique_id()] = f;
        }
        std::vector<rs2::frame> get()
        {
            std::lock_guard<std::mutex> lock(m);
            std::vector<rs2::frame> available_frames;
            std::transform(std::begin(_latest_frames),
                std::end(_latest_frames),
                std::back_inserter(available_frames),
                [](const std::pair<int, rs2::frame>& p) { return p.second; });
            return available_frames;
        }
    };
public:
    void run(const std::vector<rs2::sensor>& sensors)
    {
        window app(1280, 720, "CPP - Sensor Control Example");
        update_options_controls(sensors);

        ImGui_ImplGlfw_Init(app, false);

        while (app) // Application still alive?
        {
            ImGui_ImplGlfw_NewFrame(1);
            auto last_frames = _frames.get();
            auto total_number_of_streams = last_frames.size();
            int cols = int(std::ceil(std::sqrt(total_number_of_streams)));
            int rows = int(std::ceil(total_number_of_streams / static_cast<float>(cols)));

            float view_width = (app.width() / cols);
            float view_height = (app.height() / rows);
            int stream_no = 0;
            for (auto&& f : last_frames)
            {
                renderer.upload(colorize(f));
                rect frame_location{ view_width * (stream_no % cols), view_height * (stream_no / cols), view_width, view_height };
                if (rs2::video_frame vid_frame = f.as<rs2::video_frame>())
                {
                    rect adjuested = frame_location.adjust_ratio({ static_cast<float>(vid_frame.get_width())
                        , static_cast<float>(vid_frame.get_height()) });
                    renderer.show(adjuested);
                }
                stream_no++;
            }
            show_options_controls(sensors, app.width(), app.height());
            ImGui::Render();
        }
    }

    void on_frame(const rs2::frame& f)
    {
        _frames.put(f);
    }
private:
    texture renderer;
    rs2::colorizer colorize;
    latest_frames _frames;
    float clipping_dist = 1;
    std::vector<std::map<int, option_model>> options_metadata;
    
    void update_options_controls(const std::vector<rs2::sensor>& sensors)
    {
        for (size_t i = 0; i < sensors.size(); i++)
        {
            auto sensor = sensors[i];
            //try
            //{
            //    if (sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
            //        auto_exposure_enabled = sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) > 0;
            //}
            //catch (...)
            //{

            //}

            //try
            //{
            //    if (sensor.supports(RS2_OPTION_DEPTH_UNITS))
            //        depth_units = sensor.get_option(RS2_OPTION_DEPTH_UNITS);
            //}
            //catch (...)
            //{

            //}
            std::map<int, option_model> sensor_options;
            for (auto i = 0; i < RS2_OPTION_COUNT; i++)
            {
                option_model metadata;
                auto opt = static_cast<rs2_option>(i);

                std::stringstream ss;
                ss << "##" << sensor.get_info(RS2_CAMERA_INFO_NAME)
                    << "/" << rs2_option_to_string(opt);
                metadata.id = ss.str();
                metadata.opt = opt;
                metadata.endpoint = sensor;
                metadata.label = rs2_option_to_string(opt) + std::string("##") + ss.str();

                metadata.supported = sensor.supports(opt);
                if (metadata.supported)
                {
                    try
                    {
                        metadata.range = sensor.get_option_range(opt);
                        metadata.read_only = sensor.is_option_read_only(opt);
                        if (!metadata.read_only)
                            metadata.value = sensor.get_option(opt);
                    }
                    catch (const rs2::error& e)
                    {
                        metadata.range = { 0, 1, 0, 0 };
                        metadata.value = 0;
                        //error_message = error_to_string(e);
                    }
                }
                sensor_options[opt] = metadata;
            }
            options_metadata.push_back(sensor_options);
        }
    }
    void show_options_controls(const std::vector<rs2::sensor>& sensors, float w, float h)
    {
        const int controls_width = 100;
        ImGui::SetNextWindowSize(ImVec2(controls_width, 450), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(std::max(0, (int)(w - controls_width)), 0), ImGuiSetCond_FirstUseEver);
        if (!ImGui::Begin("Senso Option Controls"))
        {
            ImGui::End();
            return;
        }

        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(controls_width);
            ImGui::TextUnformatted("Use these controls to manipulate sensor options");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Separator();

        for (int i=0; i< options_metadata.size(); i++)
        {
            std::string sensor_name = sensors[i].get_info(RS2_CAMERA_INFO_NAME);
            ImGui::Text(sensor_name.c_str());
            for (auto&& kvp : options_metadata[i])
            {
                std::string e;
                kvp.second.draw(e);
            }
            ImGui::Separator();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
        ImGui::End();
    }
};