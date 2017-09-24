// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include "pipeline.h"
#include "stream.h"

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


    pipeline::pipeline(std::shared_ptr<librealsense::context> ctx)
        :_ctx(ctx), _hub(ctx, VID)
    {}

    std::shared_ptr<device_interface> pipeline::get_device()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (!_commited)
        {
            throw std::runtime_error(to_string() << "get_device() failed. device is not available before commiting all requests");
        } 
        return _dev;
    }

    void pipeline::commit_config()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        

        if (_config.get_requests().size() == 0 && _config.get_requests().size() == 0)
        {
            stream_profiles default_profiles;
            for (unsigned int i = 0; i < _dev->get_sensors_count(); i++)
            {
                auto&& sensor = _dev->get_sensor(i);
                auto profiles = sensor.get_stream_profiles();

                for (auto p : profiles)
                {
                    if (p->is_default())
                    {
                        default_profiles.push_back(p);
                    }
                }
            }

            // Workaround - default profiles that holds color stream shouldn't supposed to provide infrared either
            if (default_profiles.end() != std::find_if(default_profiles.begin(),
                                                       default_profiles.end(),
                                                       [](std::shared_ptr<stream_profile_interface> p)
                                                       {return p.get()->get_stream_type() == RS2_STREAM_COLOR; }))
            {
                auto it = std::find_if(default_profiles.begin(), default_profiles.end(), [](std::shared_ptr<stream_profile_interface> p){return p.get()->get_stream_type() == RS2_STREAM_INFRARED; });
                if (it != default_profiles.end())
                {
                    default_profiles.erase(it);
                }
            }

            for (auto prof : default_profiles)
            {
                auto p = dynamic_cast<video_stream_profile*>(prof.get());
                if (!p)
                {
                    throw std::runtime_error(to_string() << "stream_profile is not video_stream_profile");
                }
                enable(p->get_stream_type(), p->get_stream_index(), p->get_width(), p->get_height(), p->get_format(), p->get_framerate());
            }
        }

        if (!_dev)
        {
            auto list = _ctx->query_devices();

            for (auto dev : list)
            {

                _multistream = _config.resolve(_dev.get());
            }
        }
           

        
        _commited = true;
    }


    void pipeline::start()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (!_commited)
        {
            commit_config();
        }

        auto to_user = [&](frame_holder fref)
        {
            _queue.enqueue(std::move(fref));
        };

        frame_callback_ptr user_callback =
        { new internal_frame_callback<decltype(to_user)>(to_user),
         [](rs2_frame_callback* p) { p->release(); } };

        auto to_syncer = [&](frame_holder fref)
        {
            _syncer.invoke(std::move(fref));
        };

        frame_callback_ptr syncer_callback =
        { new internal_frame_callback<decltype(to_syncer)>(to_syncer),
            [](rs2_frame_callback* p) { p->release(); } };


        _syncer.set_output_callback(user_callback);

        _multistream.open();
        _multistream.start(syncer_callback);

        _streaming = true;
    }

    void pipeline::enable(std::string device_serial)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_commited)
        {
            throw std::runtime_error(to_string() << "enable() failed. pipeline already configured");
        }

        _dev = _hub.wait_for_device(5000, device_serial);
    }

    void pipeline::enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_commited)
        {
            throw std::runtime_error(to_string() << "enable() failed. pipeline already configured");
        }
        _config.enable_stream(stream, index, width, height, format, framerate);
    }

    void pipeline::reset_config()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_streaming)
        {
            throw std::runtime_error(to_string() << "reset_config() failed. pipeline is streaming");
        }
        
        _commited = false;
        _config.disable_all();
    }



    void pipeline::stop()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
       
        if (_streaming)
        {
            _multistream.stop();
            _multistream.close();
        }
        _streaming = false;
        
    }

   

    frame_holder pipeline::wait_for_frames(unsigned int timeout_ms)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        frame_holder f;
        if (_queue.dequeue(&f, timeout_ms))
        {
            return f;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            if (!_hub.is_connected(*_dev))
            {
                _dev = _hub.wait_for_device(timeout_ms/*, _dev.get()*/);
                _queue.clear();
                _queue.start();
                start();
                return frame_holder();
            }
            else
            {
                throw std::runtime_error(to_string() << "Frame didn't arrived within " << timeout_ms);
            }
        }
    }

    bool pipeline::poll_for_frames(frame_holder* frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (_queue.try_dequeue(frame))
        {
            return true;
        }
        return false;
    }



    stream_profiles pipeline::get_active_streams() const
    {
        stream_profiles res;
        auto profs = _multistream.get_profiles();
        for (auto p : profs)
        {
            res.push_back(p.second);
        }
        return res;
    }
    pipeline::~pipeline()
    {
        try
        {
            stop();
        }
        catch (...) {}
    }

    pipeline::resolver::resolver(std::shared_ptr<librealsense::context> ctx)
        :_ctx(ctx), _hub(ctx){}

    std::pair<std::shared_ptr<device_interface>, util::config::multistream> pipeline::resolver::resolve()
    {
        if (_device_serial.size() > 0)
        {
            auto dev = _hub.wait_for_device(5000, _device_serial);
            auto config = _config.resolve(dev.get());
            return{ dev , config };
        }
        
        for (auto dev : _ctx->query_devices())
        {
            try
            {
                auto config = _config.resolve(dev->create_device().get());
                return{ dev , config };
            }

        }
    }
}
