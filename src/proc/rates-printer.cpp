// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>
#include "rates-printer.h"

namespace librealsense
{
    bool rates_printer::should_process(const rs2::frame& frame)
    {
        return frame && !frame.is<rs2::frameset>();
    }

    rs2::frame rates_printer::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        if (_profiles.empty())
        {
            std::cout << std::endl << "#### RS Frame Rate Printer ####" << std::endl;
            _last_print_time = std::chrono::steady_clock::now();
        }
        _profiles[f.get_profile().get()].on_frame_arrival(f);
        print();
        return f;
    }

    void rates_printer::print()
    {
        auto period = std::chrono::milliseconds(1000 / _render_rate).count();
        auto curr_time = std::chrono::steady_clock::now();
        double diff = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - _last_print_time).count();

        if (diff < period)
            return;

        _last_print_time = curr_time;

        std::cout << std::fixed;
        std::cout << std::setprecision(1);
        std::cout << "\r";
        for (auto p : _profiles)
        {
            auto sp = p.second.get_stream_profile();
            std::cout << sp.stream_name() << "[" << sp.stream_index() << "]: " <<
                p.second.get_fps() << "/" << sp.fps() << " [FPS] || ";
        }
    }

    rates_printer::profile::profile() : _counter(0), _last_frame_number(0), _acctual_fps(0)
    {
    }

    int rates_printer::profile::last_frame_number()
    {
        return _last_frame_number;
    }

    rs2::stream_profile rates_printer::profile::get_stream_profile()
    {
        return _stream_profile;
    }

    float rates_printer::profile::get_fps()
    {
        return _acctual_fps;
    }

    void rates_printer::profile::on_frame_arrival(const rs2::frame& f)
    {
        if (!_stream_profile)
        {
            _stream_profile = f.get_profile();
            _last_time = std::chrono::steady_clock::now();
        }
        if (_last_frame_number >= f.get_frame_number())
            return;
        _last_frame_number = f.get_frame_number();
        auto curr_time = std::chrono::steady_clock::now();
        _time_points.push_back(curr_time);
        auto oldest = _time_points[0];
        if (_time_points.size() > _stream_profile.fps())
            _time_points.erase(_time_points.begin());
        double diff = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - oldest).count() / 1000.0;
        if (diff > 0)
            _acctual_fps = _time_points.size() / diff;
    }
}
