/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RSUTIL2_HPP
#define LIBREALSENSE_RSUTIL2_HPP

#include "rs2.hpp"
#include "rsutil2.h"

#include <map>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>

namespace rs2
{
    enum class preset
    {
        best_quality,
        largest_image,
        highest_framerate,
    };

    namespace util
    {
        template<class Dev = device>
        class Config
        {
        public:
            class multistream
            {
            public:
                multistream() {}

                explicit multistream(std::vector<typename Dev::SensorType> results,
                                     std::map<rs2_stream, typename Dev::ProfileType> profiles,
                                     std::map<rs2_stream, typename Dev::SensorType> devices)
                    : profiles(std::move(profiles)),
                      devices(std::move(devices)),
                      results(std::move(results))
                {}

                template<class T>
                void start(T callback)
                {
                    for (auto&& dev : results)
                        dev.start(callback);
                }

                void stop()
                {
                    for (auto&& dev : results)
                        dev.stop();
                }

                void close()
                {
                    for (auto&& dev : results)
                        dev.close();
                }

                rs2_intrinsics get_intrinsics(rs2_stream stream) try
                {
                    return devices.at(stream).get_intrinsics(profiles.at(stream));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have intrinsics for ") + rs2_stream_to_string(stream));
                }

                rs2_extrinsics get_extrinsics(rs2_stream from, rs2_stream to) const try
                {
                    return devices.at(from).get_extrinsics_to(from, devices.at(to), to);
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have extrinsics for ") + rs2_stream_to_string(from) + "->" + rs2_stream_to_string(to));
                }

                std::map<rs2_stream, typename Dev::ProfileType> get_profiles() const
                {
                    return profiles;
                }
            private:
                friend class Config;

                std::map<rs2_stream, typename Dev::ProfileType> profiles;
                std::map<rs2_stream, typename Dev::SensorType> devices;
                std::vector<typename Dev::SensorType> results;
            };

            Config() : require_all(true) {}

            void enable_stream(rs2_stream stream, int width, int height, int fps, rs2_format format)
            {
                _presets.erase(stream);
                _requests[stream] = { stream, width, height, fps, format };
                require_all = true;
            }

            void disable_stream(rs2_stream stream)
            {
                // disable_stream doesn't change require_all because we want both
                // enable_stream(COLOR); disable_stream(COLOR);
                // and enable_all(best_quality); disable_stream(MOTION_DATA);
                // to work as expected.
                _presets.erase(stream);
                _requests.erase(stream);
            }
            void enable_stream(rs2_stream stream, preset preset)
            {
                _requests.erase(stream);
                _presets[stream] = preset;
                require_all = true;
            }
            void enable_all(preset p)
            {
                for (int i = RS2_STREAM_DEPTH; i < RS2_STREAM_COUNT; i++)
                {
                    enable_stream(static_cast<rs2_stream>(i), p);
                }
                require_all = false;
            }

            void disable_all()
            {
                _presets.clear();
                _requests.clear();
            }

            void close(multistream stream)
            {
                for (auto&& dev : stream.results)
                    dev.close();
            }

            template <typename... Args>
            bool can_enable_stream(Dev dev, rs2_stream stream, Args... args) {
                Config<Dev> c(*this);
                c.enable_stream(stream, args...);
                for (auto && kvp : c.map_streams(dev))
                    if (kvp.second.stream == stream)
                        return true;
                return false;
            }

            multistream open(Dev dev)
            {
                auto mapping = map_streams(dev);

                // If required, make sure we've succeeded at opening
                // all the requested streams
                if (require_all) {
                    std::set<rs2_stream> all_streams;
                    for (auto && kvp : mapping)
                        all_streams.insert(kvp.second.stream_type());

                    for (auto && kvp : _presets)
                        if (!all_streams.count(kvp.first))
                            throw std::runtime_error("Config couldn't configure all streams");
                    for (auto && kvp : _requests)
                        if (!all_streams.count(kvp.first))
                            throw std::runtime_error("Config couldn't configure all streams");
                }

                // Unpack the data returned by assign
                std::map<int, std::vector<typename Dev::ProfileType> > dev_to_profiles;
                std::vector<typename Dev::SensorType> devices;
                std::map<rs2_stream, typename Dev::SensorType> stream_to_dev;
                std::map<rs2_stream, typename Dev::ProfileType> stream_to_profile;

                auto sensors = dev.query_sensors();
                for (auto && kvp : mapping) {
                    dev_to_profiles[kvp.first].push_back(kvp.second);
                    stream_to_dev.emplace(kvp.second.stream_type(), sensors[kvp.first]);
                    stream_to_profile[kvp.second.stream_type()] = kvp.second;
                }

                // TODO: make sure it works

                for (auto && kvp : dev_to_profiles) {
                    auto sub = sensors[kvp.first];
                    devices.push_back(sub);
                    sub.open(kvp.second);
                }

                return multistream(std::move(devices), std::move(stream_to_profile), std::move(stream_to_dev));
            }

        private:
            static bool sort_highest_framerate(const typename Dev::ProfileType& lhs, const typename Dev::ProfileType &rhs) {
                return lhs.fps() < rhs.fps();
            }

            static bool sort_largest_image(const typename Dev::ProfileType& lhs, const typename Dev::ProfileType &rhs) {
                if (auto a = lhs.as<video_stream_profile>())
                    if (auto b = rhs.as<video_stream_profile>())
                        return a.width()*a.height() < b.width()*b.height();
                return sort_highest_framerate(lhs, rhs);
            }

            static bool sort_best_quality(const typename Dev::ProfileType& lhs, const typename Dev::ProfileType& rhs) {
                if (auto a = lhs.as<video_stream_profile>())
                {
                    if (auto b = rhs.as<video_stream_profile>())
                    {
                        return std::make_tuple((a.width() == 640 && a.height() == 480), (lhs.fps() == 30), (lhs.format() == RS2_FORMAT_Z16), (lhs.format() == RS2_FORMAT_Y8), (lhs.format() == RS2_FORMAT_RGB8), int(lhs.format()))
                             < std::make_tuple((b.width() == 640 && b.height() == 480), (rhs.fps() == 30), (rhs.format() == RS2_FORMAT_Z16), (rhs.format() == RS2_FORMAT_Y8), (rhs.format() == RS2_FORMAT_RGB8), int(rhs.format()));

                    }
                }
                return sort_highest_framerate(lhs, rhs);
            }

            static void auto_complete(std::vector<typename Dev::ProfileType> &requests, typename Dev::SensorType &target)
            {
                auto candidates = target.get_stream_profiles();
                for (auto & request : requests)
                {
                    if (!has_wildcards(request)) continue;
                    for (auto candidate : candidates)
                    {
                        if (match(candidate, request) && !contradicts(candidate, requests))
                        {
                            request = candidate;
                            break;
                        }
                    }
                    if (has_wildcards(request))
                        throw std::runtime_error(std::string("Couldn't autocomplete request for subdevice ") + target.get_info(RS2_CAMERA_INFO_NAME));
                }
            }

            std::multimap<int, typename Dev::ProfileType> map_streams(Dev dev) const {
                std::multimap<int, typename Dev::ProfileType> out;
                std::set<rs2_stream> satisfied_streams;

                // Algorithm assumes get_adjacent_devices always
                // returns the devices in the same order
                auto devs = dev.query_sensors();
                for (size_t i = 0; i < devs.size(); ++i)
                {
                    auto sub = devs[i];
                    std::vector<typename Dev::ProfileType> targets;
                    auto profiles = sub.get_stream_profiles();

                    // deal with explicit requests
                    for (auto && kvp : _requests)
                    {
                        if (satisfied_streams.count(kvp.first)) continue; // skip satisfied requests

                        // if any profile on the subdevice can supply this request, consider it satisfiable
                        if (std::any_of(begin(profiles), end(profiles),
                            [&kvp](const typename Dev::ProfileType &profile)
                            {
                                return match(profile, kvp.second);
                            }))
                        {
                            targets.push_back(kvp.second); // store that this request is going to this subdevice
                            satisfied_streams.insert(kvp.first); // mark stream as satisfied
                        }
                    }

                    // deal with preset streams
                    std::vector<rs2_format> prefered_formats = { RS2_FORMAT_Z16, RS2_FORMAT_Y8, RS2_FORMAT_RGB8 };
                    for (auto && kvp : _presets)
                    {
                        if (satisfied_streams.count(kvp.first)) continue; // skip satisfied streams

                        auto result = [&]() -> typename Dev::ProfileType
                        {
                            switch (kvp.second)
                            {
                            case preset::best_quality:
                                std::sort(begin(profiles), end(profiles), sort_best_quality);
                                break;
                            case preset::largest_image:
                                std::sort(begin(profiles), end(profiles), sort_largest_image);
                                break;
                            case preset::highest_framerate:
                                std::sort(begin(profiles), end(profiles), sort_highest_framerate);
                                break;
                            default: throw std::runtime_error("Unknown preset selected");
                            }

                            for (auto itr = profiles.rbegin(); itr != profiles.rend(); ++itr) {
                                if (itr->stream_type() == kvp.first) {
                                    return typename Dev::ProfileType(*itr);
                                }
                            }

                            return typename Dev::ProfileType();
                        }();

                        // RS2_STREAM_COUNT signals subdevice can't handle this stream
                        if (result)
                        {
                            targets.push_back(result);
                            satisfied_streams.insert(result.stream_type());
                        }
                    }

                    if (targets.size() > 0) // if subdevice is handling any streams
                    {
                        auto_complete(targets, sub);

                        for (auto && t : targets)
                            out.emplace(i, t);

                    }
                }

                return out;

            }

            std::map<rs2_stream, typename Dev::ProfileType> _requests;
            std::map<rs2_stream, preset> _presets;
            bool require_all;
        };

        typedef Config<> config;

        /**
        * device_hub class - encapsulate the handling of connect and disconnect events
        */
        class device_hub
        {
        public:
            device_hub(const context& ctx)
                : _ctx(ctx)
            {
                _device_list = _ctx.query_devices();

                _ctx.set_devices_changed_callback([&](event_information& info)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _device_list = _ctx.query_devices();

                    // Current device will point to the first available device
                    _camera_index = 0;
                    if (_device_list.size() > 0)
                    {
                        _cv.notify_all();
                    }
                });
            }

            /**
             * If any device is connected return it, otherwise wait until next RealSense device connects.
             * Calling this method multiple times will cycle through connected devices
             */
            device wait_for_device()
            {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    // check if there is at least one device connected
                    if (_device_list.size() > 0)
                    {
                        // user can switch the devices by calling to wait_for_device until he get the desire device
                        // _camera_index is the curr device that user want to work with

                        auto dev = _device_list[_camera_index % _device_list.size()];
                        _camera_index = ++_camera_index % _device_list.size();
                        return dev;
                    }
                    else
                    {
                        std::cout<<"No device connected\n";
                    }
                }
                // if there are no devices connected or something wrong happened while enumeration
                // wait for event of device connection
                // and do it until camera connected and succeed in its creation
                while (true)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    if (_device_list.size() == 0)
                    {
                        _cv.wait_for(lock, std::chrono::hours(999999), [&]() {return _device_list.size()>0; });
                    }
                    try
                    {
                        return  _device_list[0];
                    }
                    catch (...)
                    {
                        std::cout<<"Couldn't create the device\n";
                    }
                }
            }

            /**
            * Checks if device is still connected
            */
            bool is_connected(const device& dev)
            {
                rs2_error* e = nullptr;

                if (!dev)
                    return false;

                try
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto result = rs2_device_list_contains(_device_list.get_list(), dev.get().get(), &e);
                    if (e) return false;
                    return result > 0;
                }
                catch (...)  { return false; }
            }

        private:
            const context& _ctx;
            std::mutex _mutex;
            std::condition_variable _cv;
            device_list _device_list;
            int _camera_index = 0;
        };
    }
}

#endif
