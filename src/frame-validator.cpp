// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "frame-validator.h"

#define INVALID_PIXELS_THRESHOLD 0.1

namespace librealsense
{
    void frame_validator::on_frame(rs2_frame * f)
    {
        if (!_stopped && propagate((frame_interface*)f))
        {
            if(is_user_requested_frame((frame_interface*)f))
                _user_callback->on_frame(f);
        }
    }

    void frame_validator::release()
    {}

    frame_validator::frame_validator(std::shared_ptr<sensor_interface> sensor, frame_callback_ptr user_callback, stream_profiles current_requests, stream_profiles validator_requests) :
        _sensor(sensor), 
        _user_callback(user_callback), 
        _user_requests(current_requests),
        _validator_requests(validator_requests),
        _stopped(false),
        _validated(false)
    {}

    frame_validator::~frame_validator()
    {}

    bool frame_validator::propagate(frame_interface* frame)
    {
        if(_validated)
            return true;

        auto vf = dynamic_cast<video_frame*>(frame);
        if (vf == nullptr)
            throw std::runtime_error(to_string() << "non video stream arrived to frame_validator");

        auto stream = vf->get_stream();

        //all streams except ir will be droped untill the validation on ir,
        //after the validation all streams will be passeded to user callback directly
        if (stream->get_stream_type() != RS2_STREAM_INFRARED)
            return false;

        //start to validate only from the second frame
        if(_ir_frame_num++ < 2)
            return false;

        auto w = vf->get_width();
        auto h = vf->get_height();

        auto data = static_cast<const void*>(vf->get_frame_data());

        auto invalid_pixels = 0;

        for (int y = 0; y < h*w; y++)
        {
            if (((byte*)data)[y] == 0)
            {
                invalid_pixels++;
            }
        }

        if ((float)invalid_pixels/(w*h) < INVALID_PIXELS_THRESHOLD)
        {
            _validated = true;
            return true;
        }
        else
        {

            LOG_ERROR("frame_source received corrupted frame ("<<(float)invalid_pixels/(w*h)<<"% invalid pixels), restarting the sensor...");
            auto s = _sensor;
            auto vr = _validator_requests;
            auto uc = _user_callback;
            _stopped = true;
            _reset_thread = std::thread([s, vr, uc]()
            {
                try {
                    //added delay as WA for stabilities issues
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    s->stop();
                    s->close();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    s->open(vr);
                    s->start(uc);
                }
                catch (...)
                {
                    LOG_ERROR("restarting of sensor failed");
                }
            });
            _reset_thread.detach();
        }
        return false;

    }

    bool frame_validator::is_user_requested_frame(frame_interface* frame)
    {
        return std::find_if(_user_requests.begin(), _user_requests.end(), [&](std::shared_ptr<stream_profile_interface> sp)
        {
            return stream_profiles_correspond(frame->get_stream().get(), sp.get());
        }) != _user_requests.end();
    }
}
