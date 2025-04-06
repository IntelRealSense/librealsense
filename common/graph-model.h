// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#pragma once

#include <imgui.h>  
#include <implot.h> 
#include <mutex>
#include <vector>
#include <GLFW/glfw3.h>
#include <rsutils/time/periodic-timer.h>
#include "rect.h"

#include <librealsense2/rs.hpp>

namespace rs2
{
    class graph_model
    {
    public:
        graph_model(std::string name, rs2_stream stream, bool show_n_value = true)
            : _name(name),
            _stream_type(stream),
            _show_n_value(show_n_value),
            _update_timer{ std::chrono::milliseconds(_update_rate) }
        {
            clear();
        }

        void process_frame(rs2::frame f);
        void draw(rect r);
        void clear();
        void pause();
        void resume();
        bool is_paused();
    protected:
        template<class T>
        T read_shared_data(std::function<T()> action)
        {
            std::lock_guard<std::mutex> lock(_m);
            T res = action();
            return res;
        }
        void write_shared_data(std::function<void()> action)
        {
            std::lock_guard<std::mutex> lock(_m);
            action();
        }
    private:
        float _x_value = 0.0f, _y_value = 0.0f, _z_value = 0.0f, _n_value = 0.0f;
        bool _show_n_value;
        int _update_rate = 50;
        rsutils::time::periodic_timer _update_timer;
        bool _paused = false;

        const int VECTOR_SIZE = 300;
        std::vector< float > _x_history, _y_history, _z_history, _n_history;

        std::mutex _m;
        std::string _name;

        rs2_stream _stream_type;
    };
}