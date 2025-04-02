// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include "graph-model.h"

using namespace rs2;

void graph_model::process_frame(rs2::frame f)
{
    write_shared_data(
        [&]()
        {
            if (!_paused && f && f.is< rs2::motion_frame >()
                && (f.as< rs2::motion_frame >()).get_profile().stream_type() == _stream_type)
            {
                double ts = glfwGetTime();

                if (_update_timer) // Check if timer expired
                {
                    rs2::motion_frame frame = f.as< rs2::motion_frame >();

                    // Update motion data values
                    _x_value = frame.get_motion_data().x;
                    _y_value = frame.get_motion_data().y;
                    _z_value = frame.get_motion_data().z;
                    if(_show_n_value)
                        _n_value = std::sqrt(_x_value * _x_value + _y_value * _y_value + _z_value * _z_value);

                    if (_x_history.size() >= VECTOR_SIZE)
                        _x_history.erase(_x_history.begin());
                    if (_y_history.size() >= VECTOR_SIZE)
                        _y_history.erase(_y_history.begin());
                    if (_z_history.size() >= VECTOR_SIZE)
                        _z_history.erase(_z_history.begin());
                    if (_show_n_value && _n_history.size() >= VECTOR_SIZE)
                        _n_history.erase(_n_history.begin());

                    _x_history.push_back(_x_value);
                    _y_history.push_back(_y_value);
                    _z_history.push_back(_z_value);
                    _n_history.push_back(_n_value);

                }
            }
        });
}

void graph_model::draw(rect stream_rect)
{
    ImGui::SetCursorScreenPos({ stream_rect.x, stream_rect.y });
    if (_x_history.empty()) return;

    // Set the time array
    std::vector<float> time_index(_x_history.size());
    for (size_t i = 0; i < time_index.size(); ++i)
        time_index[i] = static_cast<float>(i);

    // Read shared history data
    auto x_hist = read_shared_data<std::vector<float>>([&]() { return _x_history; });
    auto y_hist = read_shared_data<std::vector<float>>([&]() { return _y_history; });
    auto z_hist = read_shared_data<std::vector<float>>([&]() { return _z_history; });

    std::vector<float> n_hist;
    if(_show_n_value) 
        n_hist = read_shared_data<std::vector<float>>([&]() { return _n_history; });

    ImGui::BeginChild(_name.c_str(), ImVec2(stream_rect.w + 2, stream_rect.h));

    if (ImPlot::BeginPlot(_name.c_str(), ImVec2(stream_rect.w + 2, stream_rect.h)))
    {
        ImPlot::SetupAxes("Time", "Value");

        ImPlot::PlotLine("X", time_index.data(), x_hist.data(), static_cast<int>(x_hist.size()));
        ImPlot::PlotLine("Y", time_index.data(), y_hist.data(), static_cast<int>(y_hist.size()));
        ImPlot::PlotLine("Z", time_index.data(), z_hist.data(), static_cast<int>(z_hist.size()));
        if (_show_n_value)
            ImPlot::PlotLine("N", time_index.data(), n_hist.data(), static_cast<int>(n_hist.size()));

        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}

void graph_model::clear()
{
    write_shared_data(
        [&]()
        {
            _x_history.clear();
            _y_history.clear();
            _z_history.clear();
            if (_show_n_value)
                _n_history.clear();
            for (int i = 0; i < VECTOR_SIZE; i++)
            {
                _x_history.push_back(0);
                _y_history.push_back(0);
                _z_history.push_back(0);
                if(_show_n_value)
                    _n_history.push_back(0);
            }
        });
}

void graph_model::pause()
{
    _paused = true;
}

void graph_model::resume()
{
    _paused = false;
}

bool graph_model::is_paused()
{
    return _paused;
}