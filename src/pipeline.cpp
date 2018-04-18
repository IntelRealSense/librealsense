// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "proc/synthetic-stream.h"
#include "proc/syncer-processing-block.h"
#include "pipeline.h"
#include "stream.h"
#include "media/record/record_device.h"
#include "media/ros/ros_writer.h"

namespace librealsense
{
    pipeline_processing_block::pipeline_processing_block(const std::vector<int>& streams_to_aggregate) :
        _queue(new single_consumer_queue<frame_holder>(1)),
        _streams_ids(streams_to_aggregate)
    {
        auto processing_callback = [&](frame_holder frame, synthetic_source_interface* source)
        {
            handle_frame(std::move(frame), source);
        };

        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
            new internal_frame_processor_callback<decltype(processing_callback)>(processing_callback)));
    }

    void pipeline_processing_block::handle_frame(frame_holder frame, synthetic_source_interface* source)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto comp = dynamic_cast<composite_frame*>(frame.frame);
        if (comp)
        {
            for (auto i = 0; i< comp->get_embedded_frames_count(); i++)
            {
                auto f = comp->get_frame(i);
                f->acquire();
                _last_set[f->get_stream()->get_unique_id()] = f;
            }

            for (int s : _streams_ids)
            {
                if (!_last_set[s])
                    return;
            }

            std::vector<frame_holder> set;
            for (auto&& s : _last_set)
            {
                set.push_back(s.second.clone());
            }
            auto fref = source->allocate_composite_frame(std::move(set));
            if (!fref)
            {
                LOG_ERROR("Failed to allocate composite frame");
                return;
            }
            _queue->enqueue(fref);
        }
        else
        {
            LOG_ERROR("Non composite frame arrived to pipeline::handle_frame");
            assert(false);
        }
    }

    bool pipeline_processing_block::dequeue(frame_holder* item, unsigned int timeout_ms)
    {
        return _queue->dequeue(item, timeout_ms);
    }

    bool pipeline_processing_block::try_dequeue(frame_holder* item)
    {
        return _queue->try_dequeue(item);
    }

    /*

      ______   ______   .__   __.  _______  __    _______
     /      | /  __  \  |  \ |  | |   ____||  |  /  _____|
    |  ,----'|  |  |  | |   \|  | |  |__   |  | |  |  __
    |  |     |  |  |  | |  . `  | |   __|  |  | |  | |_ |
    |  `----.|  `--'  | |  |\   | |  |     |  | |  |__| |
     \______| \______/  |__| \__| |__|     |__|  \______|

    */
    pipeline_config::pipeline_config()
    {
        //empty
    }
    void pipeline_config::enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int fps)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _resolved_profile.reset();
        _stream_requests[{stream, index}] = { stream, index, width, height, format, fps };
    }

    void pipeline_config::enable_all_stream()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _resolved_profile.reset();
        _stream_requests.clear();
        _enable_all_streams = true;
    }

    void pipeline_config::enable_device(const std::string& serial)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _resolved_profile.reset();
        _device_request.serial = serial;
    }

    void pipeline_config::enable_device_from_file(const std::string& file, bool repeat_playback = true)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_device_request.record_output.empty())
        {
            throw std::runtime_error("Configuring both device from file, and record to file is unsupported");
        }
        _resolved_profile.reset();
        _device_request.filename = file;
        _playback_loop = repeat_playback;
    }

    void pipeline_config::enable_record_to_file(const std::string& file)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_device_request.filename.empty())
        {
            throw std::runtime_error("Configuring both device from file, and record to file is unsupported");
        }
        _resolved_profile.reset();
        _device_request.record_output = file;
    }

    std::shared_ptr<pipeline_profile> pipeline_config::get_cached_resolved_profile()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _resolved_profile;
    }

    void pipeline_config::disable_stream(rs2_stream stream, int index)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        auto itr = std::begin(_stream_requests);
        while (itr != std::end(_stream_requests))
        {
            //if this is the same stream type and also the user either requested all or it has the same index
            if (itr->first.first == stream && (index == -1 ||  itr->first.second == index))
            {
                itr = _stream_requests.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
        _resolved_profile.reset();
    }

    void pipeline_config::disable_all_streams()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _stream_requests.clear();
        _enable_all_streams = false;
        _resolved_profile.reset();
    }

    std::shared_ptr<pipeline_profile> pipeline_config::resolve(std::shared_ptr<pipeline> pipe, const std::chrono::milliseconds& timeout)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _resolved_profile.reset();
        auto requested_device = resolve_device_requests(pipe, timeout);

        std::vector<stream_profile> resolved_profiles;

        //if the user requested all streams, or if the requested device is from file and the user did not request any stream
        if (_enable_all_streams || (!_device_request.filename.empty() && _stream_requests.empty()))
        {
            if (!requested_device)
            {
                requested_device = pipe->wait_for_device(timeout);
            }

            util::config config;
            config.enable_all(util::best_quality);
            _resolved_profile = std::make_shared<pipeline_profile>(requested_device, config, _device_request.record_output);
            return _resolved_profile;
        }
        else
        {
            util::config config;

            //If the user did not request anything, give it the default
            if (_stream_requests.empty())
            {
                if (!requested_device)
                {
                    requested_device = pipe->wait_for_device(timeout);
                }

                auto default_profiles = get_default_configuration(requested_device);
                for (auto prof : default_profiles)
                {
                    auto p = dynamic_cast<video_stream_profile*>(prof.get());
                    if (!p)
                    {
                        LOG_ERROR("prof is not video_stream_profile");
                        throw std::logic_error("Failed to resolve request. internal error");
                    }
                    config.enable_stream(p->get_stream_type(), p->get_stream_index(), p->get_width(), p->get_height(), p->get_format(), p->get_framerate());
                }

                _resolved_profile = std::make_shared<pipeline_profile>(requested_device, config, _device_request.record_output);
                return _resolved_profile;
            }
            else
            {
                //User enabled some stream, enable only them
                for(auto&& req : _stream_requests)
                {
                    auto r = req.second;
                    config.enable_stream(r.stream, r.stream_index, r.width, r.height, r.format, r.fps);
                }

                if (!requested_device)
                {
                    //If the users did not request any device, select one for them
                    auto devs = pipe->get_context()->query_devices();
                    if (devs.empty())
                    {
                        auto dev = pipe->wait_for_device(timeout);
                        _resolved_profile = std::make_shared<pipeline_profile>(dev, config, _device_request.record_output);
                        return _resolved_profile;
                    }
                    else
                    {
                        for (auto dev_info : devs)
                        {
                            try
                            {
                                auto dev = dev_info->create_device(true);
                                _resolved_profile = std::make_shared<pipeline_profile>(dev, config, _device_request.record_output);
                                return _resolved_profile;
                            }
                            catch (...) {}
                        }
                    }

                    throw std::runtime_error("Failed to resolve request. No device found that satisfies all requirements");
                }
                else
                {
                    //User specified a device, use it with the requested configuration
                    _resolved_profile = std::make_shared<pipeline_profile>(requested_device, config, _device_request.record_output);
                    return _resolved_profile;
                }
            }
        }

        assert(0); //Unreachable code
    }

    bool pipeline_config::can_resolve(std::shared_ptr<pipeline> pipe)
    {
        try
        {    // Try to resolve from connected devices. Non-blocking call
            resolve(pipe);
            _resolved_profile.reset();
        }
        catch (const std::exception& e)
        {
            LOG_DEBUG("Config can not be resolved. " << e.what());
            return false;
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
    std::shared_ptr<device_interface> pipeline_config::get_or_add_playback_device(std::shared_ptr<pipeline> pipe, const std::string& file)
    {
        //Check if the file is already loaded to context, and if so return that device
        for (auto&& d : pipe->get_context()->query_devices())
        {
            auto playback_devs = d->get_device_data().playback_devices;
            for (auto&& p : playback_devs)
            {
                if (p.file_path == file)
                {
                    return d->create_device();
                }
            }
        }

        return pipe->get_context()->add_device(file);
    }

    std::shared_ptr<device_interface> pipeline_config::resolve_device_requests(std::shared_ptr<pipeline> pipe, const std::chrono::milliseconds& timeout)
    {
        //Prefer filename over serial
        if(!_device_request.filename.empty())
        {
            std::shared_ptr<device_interface> dev;
            try
            {
                dev = get_or_add_playback_device(pipe, _device_request.filename);
            }
            catch(const std::exception& e)
            {
                throw std::runtime_error(to_string() << "Failed to resolve request. Request to enable_device_from_file(\"" << _device_request.filename << "\") was invalid, Reason: " << e.what());
            }
            //check if a serial number was also requested, and check again the device
            if (!_device_request.serial.empty())
            {
                if (!dev->supports_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                {
                    throw std::runtime_error(to_string() << "Failed to resolve request. "
                        "Conflic between enable_device_from_file(\"" << _device_request.filename
                        << "\") and enable_device(\"" << _device_request.serial << "\"), "
                        "File does not contain a device with such serial");
                }
                else
                {
                    std::string s = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                    if (s != _device_request.serial)
                    {
                        throw std::runtime_error(to_string() << "Failed to resolve request. "
                            "Conflic between enable_device_from_file(\"" << _device_request.filename
                             << "\") and enable_device(\"" << _device_request.serial << "\"), "
                            "File contains device with different serial number (" << s << "\")");
                    }
                }
            }
            return dev;
        }

        if (!_device_request.serial.empty())
        {
            return pipe->wait_for_device(timeout, _device_request.serial);
        }

        return nullptr;
    }

    stream_profiles pipeline_config::get_default_configuration(std::shared_ptr<device_interface> dev)
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
        auto color_it = std::find_if(default_profiles.begin(), default_profiles.end(), [](std::shared_ptr<stream_profile_interface> p)
                        {
                            return p.get()->get_stream_type() == RS2_STREAM_COLOR;
                        });

        bool default_profiles_contains_color_stream = color_it != default_profiles.end();
        if (default_profiles_contains_color_stream)
        {
            auto it = std::find_if(default_profiles.begin(), default_profiles.end(), [](std::shared_ptr<stream_profile_interface> p) {return p.get()->get_stream_type() == RS2_STREAM_INFRARED; });
            if (it != default_profiles.end())
            {
                default_profiles.erase(it);
            }
        }

        return default_profiles;
    }

    bool pipeline_config::get_repeat_playback() {
        return _playback_loop;
    }

    /*
        .______    __  .______    _______  __       __  .__   __.  _______ 
        |   _  \  |  | |   _  \  |   ____||  |     |  | |  \ |  | |   ____|
        |  |_)  | |  | |  |_)  | |  |__   |  |     |  | |   \|  | |  |__   
        |   ___/  |  | |   ___/  |   __|  |  |     |  | |  . `  | |   __|  
        |  |      |  | |  |      |  |____ |  `----.|  | |  |\   | |  |____ 
        | _|      |__| | _|      |_______||_______||__| |__| \__| |_______|
    */

    pipeline::pipeline(std::shared_ptr<librealsense::context> ctx)
        :_ctx(ctx), _hub(ctx), _dispatcher(10)
    {}

    pipeline::~pipeline()
    {
        try
        {
            unsafe_stop();
        }
        catch (...) {}
    }

    std::shared_ptr<pipeline_profile> pipeline::start(std::shared_ptr<pipeline_config> conf)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_active_profile)
        {
            throw librealsense::wrong_api_call_sequence_exception("start() cannot be called before stop()");
        }
        unsafe_start(conf);
        return unsafe_get_active_profile();
    }

    std::shared_ptr<pipeline_profile> pipeline::start_with_record(std::shared_ptr<pipeline_config> conf, const std::string& file)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_active_profile)
        {
            throw librealsense::wrong_api_call_sequence_exception("start() cannot be called before stop()");
        }
        conf->enable_record_to_file(file);
        unsafe_start(conf);
        return unsafe_get_active_profile();
    }

    std::shared_ptr<pipeline_profile> pipeline::get_active_profile() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return unsafe_get_active_profile();
    }

    std::shared_ptr<pipeline_profile> pipeline::unsafe_get_active_profile() const
    {
        if (!_active_profile)
            throw librealsense::wrong_api_call_sequence_exception("get_active_profile() can only be called between a start() and a following stop()");

        return _active_profile;
    }

    void pipeline::unsafe_start(std::shared_ptr<pipeline_config> conf)
    {
        std::shared_ptr<pipeline_profile> profile = nullptr;
        //first try to get the previously resolved profile (if exists)
        auto cached_profile = conf->get_cached_resolved_profile();
        if (cached_profile)
        {
            profile = cached_profile;
        }
        else
        {
            const int NUM_TIMES_TO_RETRY = 3;
            for (int i = 1; i <= NUM_TIMES_TO_RETRY; i++)
            {
                try
                {
                    profile = conf->resolve(shared_from_this(), std::chrono::seconds(5));
                    break;
                }
                catch (...)
                {
                    if (i == NUM_TIMES_TO_RETRY)
                        throw;
                }
            }
        }

        assert(profile);
        assert(profile->_multistream.get_profiles().size() > 0);

        std::vector<int> unique_ids;
        for (auto&& s : profile->get_active_streams())
        {
            unique_ids.push_back(s->get_unique_id());
        }

        _syncer = std::unique_ptr<syncer_process_unit>(new syncer_process_unit());
        _pipeline_process = std::unique_ptr<pipeline_processing_block>(new pipeline_processing_block(unique_ids));

        auto pipeline_process_callback = [&](frame_holder fref)
        {
            _pipeline_process->invoke(std::move(fref));
        };

        frame_callback_ptr to_pipeline_process = {
            new internal_frame_callback<decltype(pipeline_process_callback)>(pipeline_process_callback),
            [](rs2_frame_callback* p) { p->release(); }
        };

        _syncer->set_output_callback(to_pipeline_process);

        auto to_syncer = [&](frame_holder fref)
        {
            _syncer->invoke(std::move(fref));
        };

        frame_callback_ptr syncer_callback = {
            new internal_frame_callback<decltype(to_syncer)>(to_syncer),
            [](rs2_frame_callback* p) { p->release(); }
        };

        auto dev = profile->get_device();
        if (auto playback = As<librealsense::playback_device>(dev))
        {
            _playback_stopped_token = playback->playback_status_changed += [this, syncer_callback](rs2_playback_status status)
            {
                if (status == RS2_PLAYBACK_STATUS_STOPPED)
                {
                    _dispatcher.invoke([this, syncer_callback](dispatcher::cancellable_timer t)
                    {
                        //If the pipeline holds a playback device, and it reached the end of file (stopped)
                        //Then we restart it
                        if (_active_profile && _prev_conf->get_repeat_playback())
                        {
                            _active_profile->_multistream.open();
                            _active_profile->_multistream.start(syncer_callback);
                        }
                    });
                }
            };
        }

        _dispatcher.start();
        profile->_multistream.open();
        profile->_multistream.start(syncer_callback);
        _active_profile = profile;
        _prev_conf = std::make_shared<pipeline_config>(*conf);
    }

    void pipeline::stop()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_active_profile)
        {
            throw librealsense::wrong_api_call_sequence_exception("stop() cannot be called before start()");
        }
        unsafe_stop();
    }

    void pipeline::unsafe_stop()
    {
        if (_active_profile)
        {
            try
            {
                auto dev = _active_profile->get_device();
                if (auto playback = As<librealsense::playback_device>(dev))
                {
                    playback->playback_status_changed -= _playback_stopped_token;
                }
                _active_profile->_multistream.stop();
                _active_profile->_multistream.close();
                _dispatcher.stop();
            }
            catch(...)
            {
            } // Stop will throw if device was disconnected. TODO - refactoring anticipated
        }
        _active_profile.reset();
        _syncer.reset();
        _pipeline_process.reset();
        _prev_conf.reset();
    }
    frame_holder pipeline::wait_for_frames(unsigned int timeout_ms)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_active_profile)
        {
            throw librealsense::wrong_api_call_sequence_exception("wait_for_frames cannot be called before start()");
        }

        frame_holder f;
        if (_pipeline_process->dequeue(&f, timeout_ms))
        {
            return f;
        }

        //hub returns true even if device already reconnected
        if (!_hub.is_connected(*_active_profile->get_device()))
        {
            try
            {
                auto prev_conf = _prev_conf;
                unsafe_stop();
                unsafe_start(prev_conf);

                if (_pipeline_process->dequeue(&f, timeout_ms))
                {
                    return f;
                }

            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(to_string() << "Device disconnected. Failed to recconect: "<<e.what() << timeout_ms);
            }
        }
        throw std::runtime_error(to_string() << "Frame didn't arrived within " << timeout_ms);
    }

    bool pipeline::poll_for_frames(frame_holder* frame)
    {
        std::lock_guard<std::mutex> lock(_mtx);

        if (!_active_profile)
        {
            throw librealsense::wrong_api_call_sequence_exception("poll_for_frames cannot be called before start()");
        }

        if (_pipeline_process->try_dequeue(frame))
        {
            return true;
        }
        return false;
    }

    std::shared_ptr<device_interface> pipeline::wait_for_device(const std::chrono::milliseconds& timeout, const std::string& serial)
    {
        // Pipeline's device selection shall be deterministic
        return _hub.wait_for_device(timeout, false, serial);
    }

    std::shared_ptr<librealsense::context> pipeline::get_context() const
    {
        return _ctx;
    }


    /*
        .______   .______        ______    _______  __   __       _______
        |   _  \  |   _  \      /  __  \  |   ____||  | |  |     |   ____|
        |  |_)  | |  |_)  |    |  |  |  | |  |__   |  | |  |     |  |__
        |   ___/  |      /     |  |  |  | |   __|  |  | |  |     |   __|
        |  |      |  |\  \----.|  `--'  | |  |     |  | |  `----.|  |____
        | _|      | _| `._____| \______/  |__|     |__| |_______||_______|
    */

    pipeline_profile::pipeline_profile(std::shared_ptr<device_interface> dev,
                                       util::config config,
                                       const std::string& to_file) :
        _dev(dev), _to_file(to_file)
    {
        if (!to_file.empty())
        {
            if (!dev)
                throw librealsense::invalid_value_exception("Failed to create a pipeline_profile, device is null");

            _dev = std::make_shared<record_device>(dev, std::make_shared<ros_writer>(to_file));
        }
        _multistream = config.resolve(_dev.get());
    }

    std::shared_ptr<device_interface> pipeline_profile::get_device()
    {
        //pipeline_profile can be retrieved from a pipeline_config and pipeline::start()
        //either way, it is created by the pipeline

        //TODO: handle case where device has disconnected and reconnected
        //TODO: remember to recreate the device as record device in case of to_file.empty() == false
        if (!_dev)
        {
            throw std::runtime_error("Device is unavailable");
        }
        return _dev;
    }

    stream_profiles pipeline_profile::get_active_streams() const
    {
        auto profiles_per_sensor = _multistream.get_profiles_per_sensor();
        stream_profiles profiles;
        for (auto&& kvp : profiles_per_sensor)
            for (auto&& p : kvp.second)
                profiles.push_back(p);

        return profiles;
    }
}
