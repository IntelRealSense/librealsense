// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_PIPELINE_HPP
#define LIBREALSENSE_RS2_PIPELINE_HPP

#include "rs_types.hpp"
#include "rs_frame.hpp"
#include "rs_context.hpp"

namespace rs2
{
    class pipeline_profile
    {
    public:
        std::vector<stream_profile> get_active_streams() const
        {
            std::vector<stream_profile> results;

            rs2_error* e = nullptr;
            std::shared_ptr<rs2_stream_profile_list> list(
                rs2_pipeline_profile_get_active_streams(_pipeline_profile.get(), &e),
                rs2_delete_stream_profiles_list);
            error::handle(e);

            auto size = rs2_get_stream_profiles_count(list.get(), &e);
            error::handle(e);

            for (auto i = 0; i < size; i++)
            {
                stream_profile profile(rs2_get_stream_profile(list.get(), i, &e));
                error::handle(e);
                results.push_back(profile);
            }

            return results;
        }

        device get_device() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_pipeline_profile_get_device(_pipeline_profile.get(), &e),
                rs2_delete_device);

            error::handle(e);

            return device(dev);
        }

    private:
        pipeline_profile(std::shared_ptr<rs2_pipeline_profile> profile = nullptr) :
            _pipeline_profile(profile)
        {

        }
        std::shared_ptr<rs2_pipeline_profile> _pipeline_profile;
        friend class configurator;
        friend class pipeline;
    };

    class configurator;
    class pipeline
    {
    public:

        pipeline(context ctx = context())
            : _ctx(ctx)
        {
            rs2_error* e = nullptr;
            _pipeline = std::shared_ptr<rs2_pipeline>(
                rs2_create_pipeline(ctx._context.get(), &e),
                rs2_delete_pipeline);
            error::handle(e);
        }

        //TBD: 
        //void add_cv_module(computer_vision_module& cv_module)
        //{
        //    rs2_error* e = nullptr;
        //    rs2_pipeline_add_cv_module(_pipeline.get(), cv_module.get(), &e);
        //    error::handle(e);
        //}

        pipeline_profile start(std::shared_ptr<rs2_configurator> config = nullptr) 
        {
            rs2_error* e = nullptr;
            auto p = std::shared_ptr<rs2_pipeline_profile>(
                rs2_pipeline_start(_pipeline.get(), config.get(), &e),
                rs2_delete_pipeline_profile);

            error::handle(e);
            return pipeline_profile(p);
        }

        pipeline_profile start(const std::string& record_to_filename, std::shared_ptr<rs2_configurator> config = nullptr)
        {
            rs2_error* e = nullptr;
            auto p = std::shared_ptr<rs2_pipeline_profile>(
                rs2_pipeline_start_with_record(_pipeline.get(), config.get(), record_to_filename.c_str(), &e),
                rs2_delete_pipeline_profile);

            error::handle(e);
            return pipeline_profile(p);
        }

        /**
        * Stop streaming
        */
        void stop() 
        {
            rs2_error* e = nullptr;
            rs2_pipeline_stop(_pipeline.get(), &e);
            error::handle(e);
        }

        /**
        * wait until new frame becomes available
        * \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
        * \return Set of coherent frames
        */
        frameset wait_for_frames(unsigned int timeout_ms = 5000) const
        {
            rs2_error* e = nullptr;
            frame f(rs2_pipeline_wait_for_frames(_pipeline.get(), timeout_ms, &e));
            error::handle(e);

            return frameset(f);
        }

        /**
        * poll if a new frame is available and dequeue if it is
        * \param[out] f - frame handle
        * \return true if new frame was stored to f
        */
        bool poll_for_frames(frameset* f) const
        {
            rs2_error* e = nullptr;
            rs2_frame* frame_ref = nullptr;
            auto res = rs2_pipeline_poll_for_frames(_pipeline.get(), &frame_ref, &e);
            error::handle(e);

            if (res) *f = frameset(frame(frame_ref));
            return res > 0;
        }

        pipeline_profile get_active_profile() const
        {
            rs2_error* e = nullptr;
            auto p = std::shared_ptr<rs2_pipeline_profile>(
                rs2_pipeline_get_active_profile(_pipeline.get(), &e),
                rs2_delete_pipeline_profile);

            error::handle(e);
            return pipeline_profile(p);
        }

    private:
        context _ctx;
        device _dev;
        std::shared_ptr<rs2_pipeline> _pipeline;
        friend class configurator;
    };

    class configurator
    {
    public:
        configurator()
        {
            rs2_error* e = nullptr;
            _config = std::shared_ptr<rs2_configurator>(
                rs2_create_configurator(&e),
                rs2_delete_configurator);
            error::handle(e);
        }

        void enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int framerate)
        {
            rs2_error* e = nullptr;
            rs2_configurator_enable_stream(_config.get(), stream, index, width, height, format, framerate, &e);
            error::handle(e);
        }

        void enable_all_streams()
        {
            rs2_error* e = nullptr;
            rs2_configurator_enable_all_stream(_config.get(), &e);
            error::handle(e);
        }

        void enable_device(const std::string& serial)
        {
            rs2_error* e = nullptr;
            rs2_configurator_enable_device(_config.get(), serial.c_str(), &e);
            error::handle(e);
        }

        void enable_device_from_file(const std::string& file_name)
        {
            rs2_error* e = nullptr;
            rs2_configurator_enable_device_from_file(_config.get(), file_name.c_str(), &e);
            error::handle(e);
        }

        void disable_stream(rs2_stream stream)
        {
            rs2_error* e = nullptr;
            rs2_configurator_disable_stream(_config.get(), stream, &e);
            error::handle(e);
        }

        void disable_all_streams()
        {
            rs2_error* e = nullptr;
            rs2_configurator_disable_all_streams(_config.get(), &e);
            error::handle(e);
        }

        pipeline_profile resolve(const pipeline& p) const
        {
            rs2_error* e = nullptr;
            auto profile = std::shared_ptr<rs2_pipeline_profile>(
                rs2_configurator_resolve(_config.get(), p._pipeline.get(), &e),
                rs2_delete_pipeline_profile);

            error::handle(e);
            return pipeline_profile(profile);
        }

        bool can_resolve(const pipeline& p) const
        {
            rs2_error* e = nullptr;
            int res = rs2_config_can_resolve(_config.get(), p._pipeline.get(), &e);
            error::handle(e);
            return res == 0 ? false : true;
        }

        operator std::shared_ptr<rs2_configurator>() const
        {
            return _config;
        }
    private:
        configurator(std::shared_ptr<rs2_configurator> config) : _config(config)
        {
        }
        std::shared_ptr<rs2_configurator> _config;

    };
}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
