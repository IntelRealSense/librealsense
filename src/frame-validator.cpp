// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "frame-validator.h"

namespace librealsense
{
    void frame_validator::on_frame(rs2_frame * f)
    {
        if (propagate((frame_interface*)f))
        {
            _user_callback->on_frame(f);
            _sensor->set_frames_callback(_user_callback);
        }
    }

    void frame_validator::release()
    {

    }

    frame_validator::frame_validator(std::shared_ptr<sensor_interface> sensor, frame_callback_ptr user_callback, stream_profiles current_requests) :
        _sensor(sensor), 
        _user_callback(user_callback), 
        _current_requests(current_requests)
    {
    
    }

    frame_validator::~frame_validator()
    {

    }

    bool frame_validator::propagate(frame_interface* frame)
    {
        auto vf = dynamic_cast<video_frame*>(frame);
        if (vf == nullptr) {
            return true;
        }
        auto stream = vf->get_stream();
        if (stream->get_stream_type() != RS2_STREAM_DEPTH)
            return false;

        auto w = vf->get_width();
        auto h = vf->get_height();
        auto bpp = vf->get_bpp() / 8;
        auto data = reinterpret_cast<const uint16_t*>(vf->get_frame_data());

        bool non_empty_pixel_found = false;

        int y_begin = h * 0.5 - h * 0.05;
        int y_end = h * 0.5 + h * 0.05;
        int x_begin = w * 0.5 - w * 0.05;
        int x_end = w * 0.5 + w * 0.05;
        for (int y = y_begin; y < y_end; y++)
        {
            for (int x = x_begin; x < x_end; x++)
            {
                auto index = x + y * w;
                if (data[index] != 0)
                {
                    non_empty_pixel_found = true;
                    break;
                }
            }
        }

        if (non_empty_pixel_found)
            return true;
        else
        {
            LOG_ERROR("frame_source received an empty depth frame, restarting the sensor...");
            auto s = _sensor;
            auto cr = _current_requests;
            auto uc = _user_callback;
            _reset_thread = std::thread([s, cr, uc]()
            {
                try {
                    s->stop();
                    s->close();
                    s->open(cr);
                    s->start(uc);
                }
                catch (...) {}
            });
            _reset_thread.detach();
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        return false;
    }
}
