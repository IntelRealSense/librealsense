// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "config.h"
#include "pipeline.h"

namespace librealsense
{
    namespace pipeline
    {
        config::config()
        {
            //empty
        }
        void config::enable_stream(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t fps)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _resolved_profile.reset();
            _stream_requests[{stream, index}] = { format, stream, index, width, height, fps };
        }

        void config::enable_all_stream()
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _resolved_profile.reset();
            _stream_requests.clear();
            _enable_all_streams = true;
        }

        void config::enable_device(const std::string& serial)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _resolved_profile.reset();
            _device_request.serial = serial;
        }

        void config::enable_device_from_file(const std::string& file, bool repeat_playback = true)
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

        void config::enable_record_to_file(const std::string& file)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_device_request.filename.empty())
            {
                throw std::runtime_error("Configuring both device from file, and record to file is unsupported");
            }
            _resolved_profile.reset();
            _device_request.record_output = file;
        }

        std::shared_ptr<profile> config::get_cached_resolved_profile()
        {
            std::lock_guard<std::mutex> lock(_mtx);
            return _resolved_profile;
        }

        void config::disable_stream(rs2_stream stream, int index)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            auto itr = std::begin(_stream_requests);
            while (itr != std::end(_stream_requests))
            {
                //if this is the same stream type and also the user either requested all or it has the same index
                if (itr->first.first == stream && (index == -1 || itr->first.second == index))
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

        void config::disable_all_streams()
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _stream_requests.clear();
            _enable_all_streams = false;
            _resolved_profile.reset();
        }

        std::shared_ptr<profile> config::resolve(std::shared_ptr<device_interface> dev)
        {
            util::config config;

            //if the user requested all streams
            if (_enable_all_streams)
            {
                for (size_t i = 0; i < dev->get_sensors_count(); ++i)
                {
                    auto&& sub = dev->get_sensor(i);
                    auto profiles = sub.get_stream_profiles(PROFILE_TAG_SUPERSET);
                    config.enable_streams(profiles);
                }
                return std::make_shared<profile>(dev, config, _device_request.record_output);
            }

            //If the user did not request anything, give it the default, on playback all recorded streams are marked as default.
            if (_stream_requests.empty())
            {
                auto default_profiles = get_default_configuration(dev);
                config.enable_streams(default_profiles);
                return std::make_shared<profile>(dev, config, _device_request.record_output);
            }

            //Enabled requested streams
            for (auto&& req : _stream_requests)
            {
                auto r = req.second;
                config.enable_stream(r.stream, r.index, r.width, r.height, r.format, r.fps);
            }
            return std::make_shared<profile>(dev, config, _device_request.record_output);
        }

        std::shared_ptr<profile> config::resolve(std::shared_ptr<pipeline> pipe, const std::chrono::milliseconds& timeout)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _resolved_profile.reset();

            //Resolve the the device that was specified by the user, this call will wait in case the device is not availabe.
            auto requested_device = resolve_device_requests(pipe, timeout);
            if (requested_device != nullptr)
            {
                _resolved_profile = resolve(requested_device);
                return _resolved_profile;
            }

            //Look for satisfy device in case the user did not specify one.
            auto devs = pipe->get_context()->query_devices(RS2_PRODUCT_LINE_ANY_INTEL);
            for (auto dev_info : devs)
            {
                try
                {
                    auto dev = dev_info->create_device(true);
                    _resolved_profile = resolve(dev);
                    return _resolved_profile;
                }
                catch (const std::exception& e)
                {
                    LOG_DEBUG("Iterate available devices - config can not be resolved. " << e.what());
                }
            }

            //If no device found wait for one
            auto dev = pipe->wait_for_device(timeout);
            if (dev != nullptr)
            {
                _resolved_profile = resolve(dev);
                return _resolved_profile;
            }

            throw std::runtime_error("Failed to resolve request. No device found that satisfies all requirements");

            assert(0); //Unreachable code
        }

        bool config::can_resolve(std::shared_ptr<pipeline> pipe)
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

        std::shared_ptr<device_interface> config::get_or_add_playback_device(std::shared_ptr<context> ctx, const std::string& file)
        {
            //Check if the file is already loaded to context, and if so return that device
            for (auto&& d : ctx->query_devices(RS2_PRODUCT_LINE_ANY))
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

            return ctx->add_device(file)->create_device(false);
        }

        std::shared_ptr<device_interface> config::resolve_device_requests(std::shared_ptr<pipeline> pipe, const std::chrono::milliseconds& timeout)
        {
            //Prefer filename over serial
            if (!_device_request.filename.empty())
            {
                std::shared_ptr<device_interface> dev;
                try
                {
                    dev = get_or_add_playback_device(pipe->get_context(), _device_request.filename);
                }
                catch (const std::exception& e)
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

        stream_profiles config::get_default_configuration(std::shared_ptr<device_interface> dev)
        {
            stream_profiles default_profiles;

            for (unsigned int i = 0; i < dev->get_sensors_count(); i++)
            {
                auto&& sensor = dev->get_sensor(i);
                auto profiles = sensor.get_stream_profiles(profile_tag::PROFILE_TAG_DEFAULT);
                default_profiles.insert(std::end(default_profiles), std::begin(profiles), std::end(profiles));
            }

            return default_profiles;
        }

        bool config::get_repeat_playback() {
            return _playback_loop;
        }
    }
}
