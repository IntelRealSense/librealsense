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
        :_ctx(ctx), _resolver(std::make_shared<resolver>(ctx))
    {}

    void pipeline::commit_config()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        _configuration = _resolver->resolve();
    }

    void pipeline::start()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (!_configuration)
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

        try
        {
            _configuration->_multistream.open();
            _configuration->_multistream.start(syncer_callback);
        }
        catch(...)
        {
            if (_configuration->device_disconnected())
            {
                reconfig();
                start();
            }
            else
            {
                throw;
            }
        }

        _streaming = true;
    }

    void pipeline::enable(std::string device_serial)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_configuration)
        {
            throw std::runtime_error(to_string() << "enable() failed. pipeline already configured");
        }

        _resolver->enable(device_serial);
    }

    void pipeline::enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_configuration)
        {
            throw std::runtime_error(to_string() << "enable() failed. pipeline already configured");
        }
        _resolver->enable(stream, index, width, height, format, framerate);
    }

    void pipeline::reset_config()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (_streaming)
        {
            throw std::runtime_error(to_string() << "reset_config() failed. pipeline is streaming");
        }

        _configuration.reset();
        _resolver = std::make_shared<resolver>(_ctx);
    }

    void pipeline::stop()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
       
        if (_streaming)
        {
            try
            {

                _configuration->_multistream.stop();
                _configuration->_multistream.close();
            }
            catch (...)
            {
            }
        }
        _streaming = false;
    }

   

    frame_holder pipeline::wait_for_frames(unsigned int timeout_ms)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (_configuration->device_disconnected())
        {
            try
            {
                reconfig(timeout_ms);
                start();
            }
            catch (...) 
            {
                return frame_holder();
            }
        }

        frame_holder f;
        if (_queue.dequeue(&f, timeout_ms))
        {
            return f;
        }

        if (_configuration->device_disconnected())
        {
            return frame_holder();
        }

        else
        {
            throw std::runtime_error(to_string() << "Frame didn't arrived within " << timeout_ms);
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


    std::shared_ptr<device_interface> pipeline::get_device()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (!_configuration)
        {
            throw std::runtime_error(to_string() << "get_device() failed. device is not available before commiting all requests");
        }
        return _configuration->_dev;
    }

    stream_profiles pipeline::get_active_streams() const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        if (!_configuration)
        {
            throw std::runtime_error(to_string() << "get_active_streams() failed. active streams are not available before commiting all requests");
        }

        stream_profiles res;
        auto profs = _configuration->_multistream.get_profiles();

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

    void pipeline::reconfig(unsigned int timeout_ms)
    {
        if (_configuration)
        {
            _configuration = _resolver->resolve(timeout_ms);
            _queue.clear();
            _queue.start();
        }
    }

    resolver::resolver(std::shared_ptr<librealsense::context> ctx)
        :_ctx(ctx), _hub(std::make_shared<device_hub>(ctx)){}

    std::shared_ptr<configuration> resolver::resolve(unsigned int timeout_ms)
    {
        std::shared_ptr<device_interface> dev;

        if (!_device_serial.empty())
        {
            dev = _hub->wait_for_device(timeout_ms, _device_serial);
        }
        else
        {
            dev = _hub->wait_for_device(timeout_ms);
        }

        if (_default_configuration || (_config.get_presets().size() == 0 && _config.get_requests().size() == 0))
        {
            set_default_configuration(dev.get());
            auto config = _config.resolve(dev.get());
            _default_configuration = true;
            return std::make_shared<configuration>(dev, config, _hub);
        }
       
        if (_device_serial.empty())
        {
            for (auto dev_info : _ctx->query_devices())
            {
                try
                {
                    auto dev = dev_info->create_device();
                    auto config = _config.resolve(dev.get());
                    return std::make_shared<configuration>(dev, config, _hub);
                }
                catch (...) {}
            }
            throw std::runtime_error("Config couldn't configure pipeline");
        }
        else
        {
            auto config = _config.resolve(dev.get());
            return std::make_shared<configuration>(dev, config, _hub);
        }
    }

    void resolver::enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        _config.enable_stream(stream, index, width, height, format, framerate);
    }

    void resolver::enable(std::string device_serial)
    {
        _device_serial = device_serial;
    }

    void resolver::set_default_configuration(device_interface* dev)
    {
        stream_profiles default_profiles;

        for (unsigned int i = 0; i < dev->get_sensors_count(); i++)
        {
            auto&& sensor = dev->get_sensor(i);
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
            auto it = std::find_if(default_profiles.begin(), default_profiles.end(), [](std::shared_ptr<stream_profile_interface> p) {return p.get()->get_stream_type() == RS2_STREAM_INFRARED; });
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

    bool configuration::device_disconnected()
    {
        return !_hub->is_connected(*_dev);
    }
}
