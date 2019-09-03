// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "sensor.h"
#include "stream.h"
#include "option.h"
namespace librealsense
{
     const int RS2_OPTION_DEPTH_INVALIDATION_ENABLE = static_cast<rs2_option>(RS2_OPTION_COUNT + 9); /**<  depth invalidation enabled*/

     class depth_invalidation_option: public ptr_option<bool>
     {
     public:
         depth_invalidation_option(bool min, bool max, bool step, bool def, bool* value, const std::string& desc):
             ptr_option<bool>(min, max, step, def, value, desc),
            _streaming(false){}

        void set_streaming(bool streaming)
        {
            _streaming = streaming;
        }

        bool is_enabled() const override
        {
            return !_streaming;
        }

     private:
         bool _streaming;
     };

    class frame_validator : public rs2_frame_callback
    {
    public:
        explicit frame_validator(std::shared_ptr<sensor_interface> sensor, frame_callback_ptr user_callback, stream_profiles user_requests, stream_profiles validator_requests);
        virtual ~frame_validator();

        void on_frame(rs2_frame * f) override;
        void release() override;

        static bool stream_profiles_correspond(stream_profile_interface* l, stream_profile_interface* r)
        {
            auto vl = dynamic_cast<video_stream_profile_interface*>(l);
            auto vr = dynamic_cast<video_stream_profile_interface*>(r);

            if (!vl || !vr)
                return false;

            return  l->get_framerate() == r->get_framerate() &&
                vl->get_width() == vr->get_width() &&
                vl->get_height() == vr->get_height();
        }

    private:
        bool propagate(frame_interface* frame);
        bool is_user_requested_frame(frame_interface* frame);

        std::thread _reset_thread;
        std::atomic<bool> _stopped;
        std::atomic<bool> _validated;
        int _ir_frame_num = 0;
        frame_callback_ptr _user_callback;
        stream_profiles _user_requests;
        stream_profiles _validator_requests;
        std::shared_ptr<sensor_interface> _sensor;
    };
}
