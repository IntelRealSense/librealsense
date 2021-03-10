// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <librealsense2/rs.hpp>
#include "../example.hpp"

namespace helper
{
    inline bool prompt_yes_no(const std::string& prompt_msg)
    {
        char ans;
        do
        {
            std::cout << prompt_msg << "[y/n]: ";
            std::cin >> ans;
            std::cout << std::endl;
        } while (!std::cin.fail() && ans != 'y' && ans != 'n');
        return ans == 'y';
    }

    inline uint32_t get_user_selection(const std::string& prompt_message)
    {
        std::cout << "\n" << prompt_message;
        uint32_t input;
        std::cin >> input;
        std::cout << std::endl;
        return input;
    }

    inline void print_separator()
    {
        std::cout << "\n======================================================\n" << std::endl;
    }

    class frame_viewer
    {
    public:
        frame_viewer(const std::string& window_title) :
            _window_title(window_title),
            _thread(new std::thread(&frame_viewer::run, this))
        {
        }
        ~frame_viewer()
        {
            if (_thread && _thread->joinable())
                _thread->join();
        }
        void operator()(rs2::frame f)
        {
            _frames.enqueue(f);
        }
        void wait()
        {
            //Wait for the windows to close
            if (_thread)
                _thread->join();
        }
    private:
        void run()
        {
            window app(640, 480, _window_title.c_str());
            std::string error;
            while (app)
            {
                float view_width = app.width();
                float view_height = app.height();
                if (error.empty())
                {
                    rs2::frame frame;
                    if (!_frames.poll_for_frame(&frame))
                    {
                        frame = _last_frame;
                    }
                    _last_frame = frame;
                    if (frame)
                    {
                        try
                        {
                            renderer.render(colorize.process(decimate.process(frame)).as<rs2::video_frame>(), { 0, 0, view_width, view_height });
                        }
                        catch (const std::exception& e)
                        {
                            error = e.what();
                        }
                    }
                }
                else
                {
                    draw_text(int(std::max(0.f, (view_width / 2) - error.length() * 3)),
                        int(view_height / 2), error.c_str());
                }
            }
        }
    private:
        texture renderer;
        rs2::colorizer colorize;
        rs2::decimation_filter decimate;
        std::string _window_title;
        rs2::frame_queue _frames;
        rs2::frame _last_frame;
        std::unique_ptr<std::thread> _thread;
    };
}
