// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include "pipeline.h"

namespace librealsense
{
    const int VID = 0x8086;
    template<class T>
    class internal_frame_callback : public rs2_frame_callback
    {
        T on_frame_function;
    public:
        explicit internal_frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame* fref) override
        {
            on_frame_function((frame_interface*)(fref));
        }

        void release() override { delete this; }
    };


    pipeline::pipeline(context& ctx)
        :_hub(ctx, VID)
    {
        _dev = _hub.wait_for_device();


    }

    void pipeline::start(frame_callback_ptr callback)
    {
        for (unsigned int i = 0; i < _dev->get_sensors_count(); i++)
        {
            auto&& sensor = _dev->get_sensor(i);
            stream_profiles default_profiles;
            auto profiles = sensor.get_stream_profiles();

            for(auto p: profiles)
            {
                if(p->is_default())
                {
                    default_profiles.push_back(p);
                }
            }
           sensor.open(default_profiles);
           sensor.start(_callback);
        }
    }

    void pipeline::enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        _config.enable_stream(stream, index, width,height,format, framerate);
        _profiles.push_back({stream, index, width, height, framerate, format});
    }

    void pipeline::disable_stream(rs2_stream stream)
    {
        _config.disable_stream(stream);

        _profiles.erase(std::find_if(_profiles.begin(), _profiles.end(), [&](stream_profile profile)
        {
            return profile.stream == stream;
        }));
    }

    void pipeline::disable_all()
    {
       _config.disable_all();
       _profiles.clear();
    }

    void pipeline::start()
    {

        auto to_syncer = [&](frame_holder fref)
        {
            _syncer.invoke(std::move(fref));
        };

        frame_callback_ptr syncer_callback =
        {new internal_frame_callback<decltype(to_syncer)>(to_syncer),
         [](rs2_frame_callback* p) { p->release(); }};

        auto to_user = [&](frame_holder fref)
        {
            _queue.enqueue(std::move(fref));
        };


        _syncer.set_output_callback({new internal_frame_callback<decltype(to_user)>(to_user),  [](rs2_frame_callback* p) { p->release(); }});


        if(_profiles.size() >0)
        {
            _multistream = _config.open(_dev.get());
            _multistream.start(syncer_callback);
        }
        else
        {
            for (unsigned int i = 0; i < _dev->get_sensors_count(); i++)
            {
                auto&& sensor = _dev->get_sensor(i);
                stream_profiles default_profiles;
                auto profiles = sensor.get_stream_profiles();

                for(auto p: profiles)
                {
                    if(p->is_default())
                    {
                        default_profiles.push_back(p);
                    }
                }
                sensor.open(default_profiles);

                sensor.start(syncer_callback);
                _sensors.push_back(&sensor);

            }
        }

    }

    frame_holder pipeline::wait_for_frames(unsigned int timeout_ms)
    {
        frame_holder f;
        if(_queue.dequeue(&f, timeout_ms))
        {
             return f;
        }

        if(!_hub.is_connected(*_dev))
        {
            _dev = _hub.wait_for_device();
            _sensors.clear();
            _queue.clear();
            _queue.start();
            start();
            return frame_holder();
        }
        else
        {
            throw std::runtime_error(to_string() <<"Frame didn't arrived within "<< timeout_ms);
        }
    }

    pipeline::~pipeline()
    {
        try
        {
            for (auto sensor : _sensors)
            {
                sensor->stop();
                sensor->close();
            }
            _multistream.stop();
            _multistream.close();
        }
        catch (...) {}
    }
}
