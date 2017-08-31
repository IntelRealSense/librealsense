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


    pipeline::pipeline(std::shared_ptr<librealsense::context> ctx, std::shared_ptr<device_interface> dev)
        :_ctx(ctx), _hub(ctx, VID), _dev(dev)
    {
        if (!dev)
            _dev = _hub.wait_for_device();

        _dev->get_device_data();
    }

    std::shared_ptr<device_interface> pipeline::get_device()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        return _dev;
    }

    std::shared_ptr<librealsense::context> pipeline::get_context()
    {
        return _ctx;
    }
    void pipeline::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        auto to_syncer = [&](frame_holder fref)
        {
            _syncer.invoke(std::move(fref));
        };

        frame_callback_ptr syncer_callback =
        { new internal_frame_callback<decltype(to_syncer)>(to_syncer),
         [](rs2_frame_callback* p) { p->release(); } };


        _syncer.set_output_callback(callback);

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

                _sensors.push_back(&sensor);
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
        _multistream = _config.open(_dev.get());
        _multistream.start(syncer_callback);
    }

    void pipeline::start()
    {
        auto to_user = [&](frame_holder fref)
        {
            _queue.enqueue(std::move(fref));
        };

        frame_callback_ptr user_callback =
        { new internal_frame_callback<decltype(to_user)>(to_user),
         [](rs2_frame_callback* p) { p->release(); } };

        start(user_callback);
    }


    void pipeline::enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        _config.enable_stream(stream, index, width, height, format, framerate);
    }

    bool pipeline::can_enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate)
    {
        return _config.can_enable_stream(_dev.get(), stream, index, width, height, format, framerate);
    }

    void pipeline::disable_stream(rs2_stream stream)
    {
        _config.disable_stream(stream);
    }

    void pipeline::disable_all()
    {
        _config.disable_all();
    }



    void pipeline::stop()
    {
        _multistream.stop();
        _multistream.close();
    }

    frame_holder pipeline::wait_for_frames(unsigned int timeout_ms)
    {
        frame_holder f;
        if (_queue.dequeue(&f, timeout_ms))
        {
            return f;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            if (!_hub.is_connected(*_dev))
            {
                _dev = _hub.wait_for_device(timeout_ms, _dev.get());
                _sensors.clear();
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
        if (_queue.try_dequeue(frame))
        {
            return true;
        }
        return false;
    }


    bool pipeline::get_extrinsics(const stream_interface& from, const stream_interface& to, rs2_extrinsics* extrinsics) const
    {
        return _ctx->try_fetch_extrinsics(from, to, extrinsics);
    }

    stream_profiles pipeline::get_selection() const
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
}
