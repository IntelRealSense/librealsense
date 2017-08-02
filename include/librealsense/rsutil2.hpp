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
            struct request_type
            {
                rs2_stream stream;
                int stream_index;
                int width, height;
                rs2_format format; 
                int fps;
            };

            struct index_type
            {
                rs2_stream stream;
                int index;

                bool operator<(const index_type& other) const
                {
                    return std::make_pair(stream, index) < 
                        std::make_pair(other.stream, other.index);
                }

                bool operator==(const index_type& other) const
                {
                    return std::make_pair(stream, index) == 
                        std::make_pair(other.stream, other.index);
                }
            };

            template<class Stream_Profile>
            static bool match(const Stream_Profile& a, const Stream_Profile& b)
            {
                if (a.stream_type() != RS2_STREAM_ANY && b.stream_type() != RS2_STREAM_ANY && (a.stream_type() != b.stream_type()))
                    return false;
                if (a.format() != RS2_FORMAT_ANY && b.format() != RS2_FORMAT_ANY && (a.format() != b.format()))
                    return false;
                if (a.fps() != 0 && b.fps() != 0 && (a.fps() != b.fps()))
                    return false;

                if (auto vid_a = a.template as<video_stream_profile>())
                {
                    if (auto vid_b = b.template as<video_stream_profile>())
                    {
                        if (vid_a.width() != 0 && vid_b.width() != 0 && (vid_a.width() != vid_b.width()))
                            return false;
                        if (vid_a.height() != 0 && vid_b.height() != 0 && (vid_a.height() != vid_b.height()))
                            return false;
                    }
                    else return false;
                }

                return true;
            }

            template<class Stream_Profile>
            static bool match(const Stream_Profile& a, const request_type& b)
            {
                if (a.stream_type() != RS2_STREAM_ANY && b.stream != RS2_STREAM_ANY && (a.stream_type() != b.stream))
                    return false;
                if (a.stream_index() != 0 && b.stream_index != 0 && (a.stream_index() != b.stream_index))
                    return false;
                if (a.format() != RS2_FORMAT_ANY && b.format != RS2_FORMAT_ANY && (a.format() != b.format))
                    return false;
                if (a.fps() != 0 && b.fps != 0 && (a.fps() != b.fps))
                    return false;

                if (auto vid_a = a.template as<video_stream_profile>())
                {
                    if (vid_a.width() != 0 && b.width != 0 && (vid_a.width() != b.width))
                        return false;
                    if (vid_a.height() != 0 && b.height != 0 && (vid_a.height() != b.height))
                        return false;
                }

                return true;
            }

            template<class StreamProfile>
            static bool contradicts(const StreamProfile& a, const std::vector<StreamProfile>& others)
            {
                for (auto request : others)
                {
                    if (a.fps() != 0 && request.fps() != 0 && (a.fps() != request.fps()))
                        return true;
                }

                if (auto vid_a = a.template as<video_stream_profile>())
                {
                    for (auto request : others)
                    {
                        if (auto vid_b = request.template as<video_stream_profile>())
                        {
                            if (vid_a.width() != 0 && vid_b.width() != 0 && (vid_a.width() != vid_b.width()))
                                return true;
                            if (vid_a.fps() != 0 && vid_b.fps() != 0 && (vid_a.fps() != vid_b.fps()))
                                return true;
                        }
                    }
                }

                return false;
            }

            template<class StreamProfile>
            static bool contradicts(const StreamProfile& a, const std::vector<request_type>& others)
            {
                for (auto request : others)
                {
                    if (a.fps() != 0 && request.fps != 0 && (a.fps() != request.fps))
                        return true;
                }

                if (auto vid_a = a.template as<video_stream_profile>())
                {
                    for (auto request : others)
                    {
                        if (vid_a.width() != 0 && request.width != 0 && (vid_a.width() != request.width))
                            return true;
                        if (vid_a.height() != 0 && request.height != 0 && (vid_a.height() != request.height))
                            return true;
                    }
                }

                return false;
            }

            template<class Stream_Profile>
            static bool has_wildcards(const Stream_Profile& a)
            {
                if (a.fps() == 0 || a.stream_type() == RS2_STREAM_ANY || a.format() == RS2_FORMAT_ANY) return true;
                if (auto vid_a = a.template as<video_stream_profile>())
                {
                    if (vid_a.width() == 0 || vid_a.height() == 0) return true;
                }
                return false;
            }

            template<>
            static bool has_wildcards(const request_type& a)
            {
                if (a.fps == 0 || a.stream == RS2_STREAM_ANY || a.format == RS2_FORMAT_ANY) return true;
                if (a.width == 0 || a.height == 0) return true;
                return false;
            }

            class multistream
            {
            public:
                multistream() {}

                explicit multistream(std::vector<typename Dev::SensorType> results,
                                     std::map<index_type, typename Dev::ProfileType> profiles,
                                     std::map<index_type, typename Dev::SensorType> devices)
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

                rs2_intrinsics get_intrinsics(rs2_stream stream, int index = 0) try
                {
                    return profiles.at({ stream, index }).template as<video_stream_profile>().get_intrinsics();
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have intrinsics for ") + rs2_stream_to_string(stream));
                }

                rs2_extrinsics get_extrinsics(rs2_stream from, rs2_stream to) const try
                {
                    return profiles.at({ from, 0 }).get_extrinsics_to(profiles.at({ to, 0 }));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have extrinsics for ") + rs2_stream_to_string(from) + "->" + rs2_stream_to_string(to));
                }

                rs2_extrinsics get_extrinsics(index_type from, index_type to) const try
                {
                    return profiles.at(from).get_extrinsics_to(profiles.at(to));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have extrinsics for ") + rs2_stream_to_string(from) + "->" + rs2_stream_to_string(to));
                }

                std::map<index_type, typename Dev::ProfileType> get_profiles() const
                {
                    return profiles;
                }
            private:
                friend class Config;

                std::map<index_type, typename Dev::ProfileType> profiles;
                std::map<index_type, typename Dev::SensorType> devices;
                std::vector<typename Dev::SensorType> results;
            };

            Config() : require_all(true) {}

            void enable_stream(rs2_stream stream, int width, int height, rs2_format format, int fps)
            {
                _presets.erase({ stream, 0 });
                _requests[{stream, 0}] = request_type{ stream, 0, width, height, format, fps };
                require_all = true;
            }

            void enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int fps)
            {
                _presets.erase({ stream, index });
                _requests[{stream, index}] = request_type{ stream, index, width, height, format, fps };
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
                _requests.erase({ stream, 0 });
                _presets[{ stream, 0 }] = preset;
                require_all = true;
            }

            void enable_stream(rs2_stream stream, int index, preset preset)
            {
                _requests.erase(stream);
                _presets[{stream, index}] = preset;
                require_all = true;
            }

            void enable_all(preset p)
            {
                for (int i = RS2_STREAM_DEPTH; i < RS2_STREAM_COUNT; i++)
                {
                    enable_stream(static_cast<rs2_stream>(i), 0, p);
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
                    if (kvp.second.stream_type() == stream)
                        return true;
                return false;
            }

            multistream open(Dev dev)
            {
                 auto mapping = map_streams(dev);

                // If required, make sure we've succeeded at opening
                // all the requested streams
                if (require_all) {
                    std::set<index_type> all_streams;
                    for (auto && kvp : mapping)
                        all_streams.insert({ kvp.second.stream_type(), kvp.second.stream_index() });

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
                std::map<index_type, typename Dev::SensorType> stream_to_dev;
                std::map<index_type, typename Dev::ProfileType> stream_to_profile;

                auto sensors = dev.query_sensors();
                for (auto && kvp : mapping) {
                    dev_to_profiles[kvp.first].push_back(kvp.second);
                    index_type idx{ kvp.second.stream_type(), kvp.second.stream_index() };
                    stream_to_dev.emplace(idx, sensors[kvp.first]);
                    stream_to_profile[idx] = kvp.second;
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
                if (auto a = lhs.template as<video_stream_profile>())
                    if (auto b = rhs.template as<video_stream_profile>())
                        return a.width()*a.height() < b.width()*b.height();
                return sort_highest_framerate(lhs, rhs);
            }

            static bool sort_best_quality(const typename Dev::ProfileType& lhs, const typename Dev::ProfileType& rhs) {
                if (auto a = lhs.template as<video_stream_profile>())
                {
                    if (auto b = rhs.template as<video_stream_profile>())
                    {
                        return std::make_tuple((a.width() == 640 && a.height() == 480), (lhs.fps() == 30), (lhs.format() == RS2_FORMAT_Z16), (lhs.format() == RS2_FORMAT_Y8), (lhs.format() == RS2_FORMAT_RGB8), int(lhs.format()))
                             < std::make_tuple((b.width() == 640 && b.height() == 480), (rhs.fps() == 30), (rhs.format() == RS2_FORMAT_Z16), (rhs.format() == RS2_FORMAT_Y8), (rhs.format() == RS2_FORMAT_RGB8), int(rhs.format()));

                    }
                }
                return sort_highest_framerate(lhs, rhs);
            }

            static void auto_complete(std::vector<request_type> &requests, typename Dev::SensorType &target)
            {
                auto candidates = target.get_stream_profiles();
                for (auto & request : requests)
                {
                    if (!has_wildcards(request)) continue;
                    for (auto candidate : candidates)
                    {
                        if (match(candidate, request) && !contradicts(candidate, requests))
                        {
                            request = to_request(candidate);
                            break;
                        }
                    }
                    if (has_wildcards(request))
                        throw std::runtime_error(std::string("Couldn't autocomplete request for subdevice ") + target.get_info(RS2_CAMERA_INFO_NAME));
                }
            }

            static request_type to_request(typename Dev::ProfileType profile)
            {
                request_type r;
                r.fps = profile.fps();
                r.stream = profile.stream_type();
                r.format = profile.format();
                r.stream_index = profile.stream_index();
                if (auto vid = profile.template as<video_stream_profile>())
                {
                    r.width = vid.width();
                    r.height = vid.height();
                }
                else
                {
                    r.width = 1;
                    r.height = 1;
                }
                return r;
            }

            std::multimap<int, typename Dev::ProfileType> map_streams(Dev dev) const 
            {
                std::multimap<int, typename Dev::ProfileType> out;
                std::set<index_type> satisfied_streams;

                // Algorithm assumes get_adjacent_devices always
                // returns the devices in the same order
                auto devs = dev.query_sensors();
                for (size_t i = 0; i < devs.size(); ++i)
                {
                    auto sub = devs[i];
                    std::vector<request_type> targets;
                    auto profiles = sub.get_stream_profiles();

                    // deal with explicit requests
                    for (auto && kvp : _requests)
                    {
                        if (satisfied_streams.count(kvp.first)) continue; // skip satisfied requests

                        // if any profile on the subdevice can supply this request, consider it satisfiable
                        auto it = std::find_if(begin(profiles), end(profiles),
                            [&kvp](const typename Dev::ProfileType &profile)
                        {
                            return match(profile, kvp.second);
                        });
                        if (it != end(profiles))
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

                        auto result = [&]() -> request_type
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

                            for (auto itr = profiles.rbegin(); itr != profiles.rend(); ++itr) 
                            {
                                if (itr->stream_type() == kvp.first.stream && itr->stream_index() == kvp.first.index) {
                                    return to_request(typename Dev::ProfileType(*itr));
                                }
                            }

                            return { RS2_STREAM_ANY, 0, 0, 0, RS2_FORMAT_ANY, 0 };
                        }();

                        // RS2_STREAM_COUNT signals subdevice can't handle this stream
                        if (result.stream != RS2_STREAM_ANY)
                        {
                            targets.push_back(result);
                            satisfied_streams.insert({ result.stream, result.stream_index });
                        }
                    }

                    if (targets.size() > 0) // if subdevice is handling any streams
                    {
                        auto_complete(targets, sub);

                        for (auto && t : targets)
                        {
                            for (auto && p : profiles)
                            {
                                if (match(p, t))
                                {
                                    out.emplace(i, p);
                                    break;
                                }
                            }
                        }
                            
                    }
                }

                return out;

            }

            std::map<index_type, request_type> _requests;
            std::map<index_type, preset> _presets;
            bool require_all;
        };

        typedef Config<> config;

        /**
        * device_hub class - encapsulate the handling of connect and disconnect events
        */
        class device_hub
        {
        public:
            explicit device_hub(const context& ctx)
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
