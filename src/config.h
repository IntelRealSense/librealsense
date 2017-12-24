// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>
#include "sensor.h"
#include "types.h"

namespace librealsense
{

    namespace util
    {
        enum config_preset
        {
            best_quality,
            largest_image,
            highest_framerate,
        };

        class config
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
            };



            static bool match_stream(const index_type& a, const index_type& b)
            {
                if (a.stream != RS2_STREAM_ANY && b.stream != RS2_STREAM_ANY && (a.stream != b.stream))
                    return false;
                if (a.index != -1 && b.index != -1 && (a.index != b.index))
                    return false;

                return true;
            }

            template<class Stream_Profile>
            static bool match(const Stream_Profile& a, const Stream_Profile& b)
            {
                if (a.stream_type() != RS2_STREAM_ANY && b.stream_type() != RS2_STREAM_ANY && (a.stream_type() != b.stream_type()))
                    return false;
                 if (a.stream_index() != -1 && b.stream_index() != -1 && (a.stream_index() != b.stream_index()))
                     return false;
                if (a.format() != RS2_FORMAT_ANY && b.format() != RS2_FORMAT_ANY && (a.format() != b.format()))
                    return false;
                if (a.fps() != 0 && b.fps() != 0 && (a.fps() != b.fps()))
                    return false;


                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    if (auto vid_b = dynamic_cast<video_stream_profile_interface*>(b))
                    {
                        if (vid_a->get_width() != 0 && vid_b->get_width() != 0 && (vid_a->get_width() != vid_b->get_width()))
                            return false;
                        if (vid_a->get_height() != 0 && vid_b->get_height() != 0 && (vid_a->get_height() != vid_b->get_height()))
                            return false;
                    }
                    else return false;
                }

                return true;
            }

            static bool match(stream_profile_interface* a, const request_type& b)
            {
                if (a->get_stream_type() != RS2_STREAM_ANY && b.stream != RS2_STREAM_ANY && (a->get_stream_type() != b.stream))
                    return false;
                if (a->get_stream_index() != -1 && b.stream_index != -1 && (a->get_stream_index() != b.stream_index))
                    return false;
                if (a->get_format() != RS2_FORMAT_ANY && b.format != RS2_FORMAT_ANY && (a->get_format() != b.format))
                    return false;
                if (a->get_framerate() != 0 && b.fps != 0 && (a->get_framerate() != b.fps))
                    return false;

                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    if (vid_a->get_width() != 0 && b.width != 0 && (vid_a->get_width() != b.width))
                        return false;
                    if (vid_a->get_height() != 0 && b.height != 0 && (vid_a->get_height() != b.height))
                        return false;
                }

                return true;
            }


            static bool contradicts(stream_profile_interface* a, stream_profiles others)
            {
                for (auto request : others)
                {
                    if (a->get_framerate()!= 0 && request->get_framerate() != 0 && (a->get_framerate() != request->get_framerate()))
                        return true;
                }

                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    for (auto request : others)
                    {
                        if (auto vid_b = dynamic_cast<video_stream_profile_interface*>(request.get()))
                        {
                            if (vid_a->get_width() != 0 && vid_b->get_width() != 0 && (vid_a->get_width() != vid_b->get_width()))
                                return true;
                            if (vid_a->get_framerate() != 0 && vid_b->get_framerate() != 0 && (vid_a->get_framerate() != vid_b->get_framerate()))
                                return true;
                        }
                    }
                }

                return false;
            }

            static bool contradicts(stream_profile_interface* a, const std::vector<request_type>& others)
            {
                for (auto request : others)
                {
                    if (a->get_framerate() != 0 && request.fps != 0 && (a->get_framerate() != request.fps))
                        return true;
                }

                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    for (auto request : others)
                    {
                        // Patch for DS5U_S that allows different resolutions on multi-pin device
                        if ((vid_a->get_height() == vid_a->get_width()) && (request.height == request.width))
                            return false;
                        if (vid_a->get_width() != 0 && request.width != 0 && (vid_a->get_width() != request.width))
                            return true;
                        if (vid_a->get_height() != 0 && request.height != 0 && (vid_a->get_height() != request.height))
                            return true;
                    }
                }

                return false;
            }


            static bool has_wildcards(stream_profile_interface* a)
            {
                if (a->get_framerate() == 0 || a->get_stream_type() == RS2_STREAM_ANY || a->get_stream_index() == -1 || a->get_format() == RS2_FORMAT_ANY) return true;
                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    if (vid_a->get_width() == 0 || vid_a->get_height() == 0) return true;
                }
                return false;
            }

            static bool has_wildcards(const request_type& a)
            {
                if (a.fps == 0 || a.stream == RS2_STREAM_ANY || a.format == RS2_FORMAT_ANY) return true;
                if (a.width == 0 || a.height == 0) return true;
                if (a.stream_index == -1) return true;
                return false;
            }
            std::map<index_type, request_type> get_requests()
            {
                return _requests;
            }

            std::map<index_type, config_preset> get_presets()
            {
                return _presets;
            }

            class multistream
            {
            public:
                multistream() {}

                explicit multistream(std::map<int, sensor_interface*> results,
                                     std::map<index_type, std::shared_ptr<stream_profile_interface>> profiles,
                                     std::map<int, stream_profiles> dev_to_profiles)
                        :   _profiles(std::move(profiles)),
                            _dev_to_profiles(std::move(dev_to_profiles)),
                            _results(std::move(results))
                {}


                void open()
                {
                    for (auto && kvp : _dev_to_profiles) {
                        auto&& sub = _results.at(kvp.first);
                        sub->open(kvp.second);
                    }
                }

                template<class T>
                void start(T callback)
                {
                    for (auto&& sensor : _results)
                        sensor.second->start(callback);
                }

                void stop()
                {
                    for (auto&& sensor : _results)
                        sensor.second->stop();
                }

                void close()
                {
                    for (auto&& sensor : _results)
                        sensor.second->close();
                }
                std::map<index_type, std::shared_ptr<stream_profile_interface>> get_profiles() const
                {
                    return _profiles;
                }

                std::map<int, stream_profiles> get_profiles_per_sensor() const
                {
                    return _dev_to_profiles;
                }
            private:
                friend class config;

                std::map<index_type, std::shared_ptr<stream_profile_interface>> _profiles;
                std::map<index_type, sensor_interface*> _devices;
                std::map<int, sensor_interface*> _results;
                std::map<int, stream_profiles> _dev_to_profiles;
            };

            config() : require_all(true) {}


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

                auto itr = _requests.begin();
                while( itr != _requests.end())
                {
                    if(itr->first.stream == stream)
                    {
                        _requests.erase(itr++);
                    }
                    else
                    {
                        itr++;
                    }
                }

            }
            void enable_stream(rs2_stream stream, config_preset preset)
            {
                _requests.erase({ stream, -1 });
                _presets[{ stream, -1 }] = preset;
                require_all = true;
            }

            void enable_stream(rs2_stream stream, int index, config_preset preset)
            {
                _requests.erase({stream, index});
                _presets[{stream, index}] = preset;
                require_all = true;
            }

            void enable_all(config_preset p)
            {
                for (int i = RS2_STREAM_DEPTH; i < RS2_STREAM_COUNT; i++)
                {
                    enable_stream(static_cast<rs2_stream>(i), -1, p);
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
                for (auto&& sensor : stream._results)
                    sensor.second->close();
            }

         /*   template <typename... Args>
            bool can_enable_stream(device_interface* dev, rs2_stream stream, Args... args) {
                config c(*this);
                c.enable_stream(stream, args...);
                for (auto && kvp : c.map_streams(dev))
                    if (kvp.second->get_stream_type() == stream)
                        return true;
                return false;
            }*/

           bool can_enable_stream(device_interface* dev, rs2_stream stream, int index, int width, int height, rs2_format format, int fps)
           {
               config c(*this);
               c.enable_stream(stream, index, width, height, format, fps);
               for (auto && kvp : c.map_streams(dev))

                   if (kvp.second->get_stream_type() == stream && kvp.second->get_stream_index() == index)
                       return true;

               auto it = _requests.erase({stream, index});
               return false;
           }

            multistream resolve(device_interface* dev)
            {
                 auto mapping = map_streams(dev);

                // If required, make sure we've succeeded at opening
                // all the requested streams
                if (require_all)
                {
                    std::set<index_type> all_streams;
                    for (auto && kvp : mapping)
                        all_streams.insert({ kvp.second->get_stream_type(), kvp.second->get_stream_index() });


                    for (auto && kvp : _presets)
                    {
                        auto it = std::find_if(std::begin(all_streams), std::end(all_streams), [&](const index_type& i)
                        {
                            return match_stream(kvp.first, i);
                        });
                        if (it == std::end(all_streams))
                            throw std::runtime_error("Config couldn't configure all streams");
                    }
                    for (auto && kvp : _requests)
                    {
                        auto it = std::find_if(std::begin(all_streams), std::end(all_streams), [&](const index_type& i)
                        {
                            return match_stream(kvp.first, i);
                        });
                        if (it == std::end(all_streams))
                            throw std::runtime_error("Config couldn't configure all streams");
                    }
                }

                // Unpack the data returned by assign
                std::map<int, stream_profiles> dev_to_profiles;
                std::map<index_type, sensor_interface*> stream_to_dev;
                std::map<index_type, std::shared_ptr<stream_profile_interface>> stream_to_profile;

                std::map<int, sensor_interface*> sensors_map;
                for(auto i = 0; i< dev->get_sensors_count(); i++)
                {
                    if (mapping.find(i) != mapping.end())
                    {
                        sensors_map[i] = &dev->get_sensor(i);
                    }
                }

                for (auto && kvp : mapping) {
                    dev_to_profiles[kvp.first].push_back(kvp.second);
                    index_type idx{ kvp.second->get_stream_type(), kvp.second->get_stream_index() };
                    stream_to_dev.emplace(idx, sensors_map.at(kvp.first));
                    stream_to_profile[idx] = kvp.second;
                }

                // TODO: make sure it works
                return multistream(std::move(sensors_map), std::move(stream_to_profile), std::move(dev_to_profiles));
            }

        private:
            static bool sort_highest_framerate(const std::shared_ptr<stream_profile_interface> lhs, const std::shared_ptr<stream_profile_interface> rhs) {
                return lhs->get_framerate() < rhs->get_framerate();
            }

            static bool sort_largest_image(std::shared_ptr<stream_profile_interface> lhs, std::shared_ptr<stream_profile_interface> rhs) {
                if (auto a = dynamic_cast<video_stream_profile_interface*>(lhs.get()))
                    if (auto b = dynamic_cast<video_stream_profile_interface*>(rhs.get()))
                        return a->get_width()*a->get_height() < b->get_width()*b->get_height();
                return sort_highest_framerate(lhs, rhs);
            }

            static bool sort_best_quality( std::shared_ptr<stream_profile_interface> lhs, std::shared_ptr<stream_profile_interface> rhs) {
                if (auto a = dynamic_cast<video_stream_profile_interface*>(lhs.get()))
                {
                    if (auto b = dynamic_cast<video_stream_profile_interface*>(rhs.get()))
                    {
                        return std::make_tuple((a->get_height() == 640 && a->get_height() == 480), (lhs->get_framerate() == 30), (lhs->get_format() == RS2_FORMAT_Z16), (lhs->get_format() == RS2_FORMAT_Y8), (lhs->get_format() == RS2_FORMAT_RGB8), int(lhs->get_format()))
                             < std::make_tuple((b->get_width() == 640 && b->get_height() == 480), (rhs->get_framerate() == 30), (rhs->get_format() == RS2_FORMAT_Z16), (rhs->get_format() == RS2_FORMAT_Y8), (rhs->get_format() == RS2_FORMAT_RGB8), int(rhs->get_format()));

                    }
                }
                return sort_highest_framerate(lhs, rhs);
            }

            static void auto_complete(std::vector<request_type> &requests, sensor_interface &target)
            {
                auto candidates = target.get_stream_profiles();
                for (auto & request : requests)
                {
                    if (!has_wildcards(request)) continue;
                    for (auto candidate : candidates)
                    {
                        if (match(candidate.get(), request) && !contradicts(candidate.get(), requests))
                        {
                            request = to_request(candidate.get());
                            break;
                        }
                    }
                    if (has_wildcards(request))
                        throw std::runtime_error(std::string("Couldn't autocomplete request for subdevice ") + target.get_info(RS2_CAMERA_INFO_NAME));
                }
            }

            static request_type to_request(stream_profile_interface* profile)
            {
                request_type r;
                r.fps = profile->get_framerate();
                r.stream = profile->get_stream_type();
                r.format = profile->get_format();
                r.stream_index = profile->get_stream_index();
                if (auto vid = dynamic_cast<video_stream_profile_interface*>(profile))
                {
                    r.width = vid->get_width();
                    r.height = vid->get_height();
                }
                else
                {
                    r.width = 1;
                    r.height = 1;
                }
                return r;
            }

            std::multimap<int, std::shared_ptr<stream_profile_interface>> map_streams(device_interface* dev) const
            {
                std::multimap<int, std::shared_ptr<stream_profile_interface>> out;
                std::set<index_type> satisfied_streams;

                // Algorithm assumes get_adjacent_devices always
                // returns the devices in the same order
                for (size_t i = 0; i < dev->get_sensors_count(); ++i)
                {
                    auto&& sub = dev->get_sensor(i);
                    std::vector<request_type> targets;
                    auto profiles = sub.get_stream_profiles();

                    // deal with explicit requests
                    for (auto && kvp : _requests)
                    {
                        if (satisfied_streams.count(kvp.first)) continue; // skip satisfied requests

                        // if any profile on the subdevice can supply this request, consider it satisfiable
                        auto it = std::find_if(begin(profiles), end(profiles),
                            [&kvp](const std::shared_ptr<stream_profile_interface>& profile)
                        {
                            return match(profile.get(), kvp.second);
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
                            case config_preset::best_quality:
                                std::sort(begin(profiles), end(profiles), sort_best_quality);
                                break;
                            case config_preset::largest_image:
                                std::sort(begin(profiles), end(profiles), sort_largest_image);
                                break;
                            case config_preset::highest_framerate:
                                std::sort(begin(profiles), end(profiles), sort_highest_framerate);
                                break;
                            default: throw std::runtime_error("Unknown preset selected");
                            }

                            for (auto itr: profiles)
                            {
                                auto stream = index_type{ itr->get_stream_type() ,itr->get_stream_index() };
                                if (match_stream(stream, kvp.first))
                                {
                                    return to_request(itr.get());
                                }
                            }

                            return { RS2_STREAM_ANY, -1, 0, 0, RS2_FORMAT_ANY, 0 };
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
                                if (match(p.get(), t))
                                {
                                    out.emplace((int)i, p);
                                    break;
                                }
                            }
                        }

                    }
                }

                return out;

            }

            std::map<index_type, request_type> _requests;
            std::map<index_type, config_preset> _presets;
            bool require_all;
        };

    }
}

#endif
