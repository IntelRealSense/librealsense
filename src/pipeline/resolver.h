// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>
#include <set>
#include "sensor.h"
#include "types.h"
#include "stream.h"

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
                uint32_t width, height;
                rs2_format format;
                uint32_t fps;
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
                if (auto vid_a = dynamic_cast<video_stream_profile_interface*>(a))
                {
                    for (auto request : others)
                    {
                        if (a->get_framerate() != 0 && request.fps != 0 && (a->get_framerate() != request.fps))
                            return true;
                    }

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

            void enable_streams(stream_profiles profiles)
            {
                std::map<std::tuple<int, int>, std::vector<std::shared_ptr<stream_profile_interface>>> profiles_map;

                for (auto profile : profiles)
                {
                    profiles_map[std::make_tuple(profile->get_unique_id(), profile->get_stream_index())].push_back(profile);
                }

                for (auto profs : profiles_map)
                {
                    std::sort(begin(profs.second), end(profs.second), sort_best_quality);
                    auto p = profs.second.front().get();
                    auto vp = dynamic_cast<video_stream_profile*>(p);
                    if (vp)
                    {
                        enable_stream(vp->get_stream_type(), vp->get_stream_index(), vp->get_width(), vp->get_height(), vp->get_format(), vp->get_framerate());
                        continue;
                    }
                    enable_stream(p->get_stream_type(), p->get_stream_index(), 0, 0, p->get_format(), p->get_framerate());
                }
            }

            void enable_stream(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t fps)
            {
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

            void disable_all()
            {
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
                        return a->get_width()*a->get_height() > b->get_width()*b->get_height();
                return sort_highest_framerate(lhs, rhs);
            }

            static bool is_best_format(rs2_stream stream, rs2_format format)
            {
                switch (stream)
                {
                case rs2_stream::RS2_STREAM_COLOR: return format == RS2_FORMAT_RGB8;
                case rs2_stream::RS2_STREAM_DEPTH: return format == RS2_FORMAT_Z16;
                case rs2_stream::RS2_STREAM_INFRARED: return format == RS2_FORMAT_Y8;
                }
                return false;
            }

            static bool sort_best_quality(std::shared_ptr<stream_profile_interface> lhs, std::shared_ptr<stream_profile_interface> rhs) {
                if (auto a = dynamic_cast<video_stream_profile_interface*>(lhs.get()))
                {
                    if (auto b = dynamic_cast<video_stream_profile_interface*>(rhs.get()))
                    {
                        return std::make_tuple(a->get_width() == 640 && a->get_height() == 480, a->get_framerate() == 30, is_best_format(a->get_stream_type(), a->get_format()))
                            > std::make_tuple(b->get_width() == 640 && b->get_height() == 480, b->get_framerate() == 30, is_best_format(b->get_stream_type(), b->get_format()));
                    }
                }
                return sort_highest_framerate(lhs, rhs);
            }

            static void auto_complete(std::vector<request_type> &requests, stream_profiles candidates)
            {
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
                        throw std::runtime_error(std::string("Couldn't autocomplete request for subdevice"));
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

            stream_profiles map_sub_device(stream_profiles profiles, std::set<index_type> satisfied_streams) const
            {
                stream_profiles rv;
                try
                {
                    std::vector<request_type> targets;

                    // deal with explicit requests
                    for (auto && kvp : _requests)
                    {
                        if (satisfied_streams.count(kvp.first)) continue; // skip satisfied requests

                         // if any profile on the subdevice can supply this request, consider it satisfiable
                        auto it = std::find_if(begin(profiles), end(profiles), [&kvp](const std::shared_ptr<stream_profile_interface>& profile)
                        {
                            return match(profile.get(), kvp.second);
                        });
                        if (it != end(profiles))
                        {
                            targets.push_back(kvp.second); // store that this request is going to this subdevice
                            satisfied_streams.insert(kvp.first); // mark stream as satisfied
                        }
                    }

                    if (targets.size() > 0) // if subdevice is handling any streams
                    {
                        auto_complete(targets, profiles);

                        for (auto && t : targets)
                        {
                            for (auto && p : profiles)
                            {
                                if (match(p.get(), t))
                                {
                                    rv.push_back(p);
                                    break;
                                }
                            }
                        }
                    }
                }
                catch (std::exception e)
                {
                    LOG_ERROR(e.what());
                }
                return rv;
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

                    auto default_profiles = map_sub_device(sub.get_stream_profiles(profile_tag::PROFILE_TAG_SUPERSET), satisfied_streams);
                    auto any_profiles = map_sub_device(sub.get_stream_profiles(profile_tag::PROFILE_TAG_ANY), satisfied_streams);

                    //use any streams if default streams wasn't satisfy
                    auto profiles = default_profiles.size() == any_profiles.size() ? default_profiles : any_profiles;

                    for (auto p : profiles)
                        out.emplace((int)i, p);
                }

                if(_requests.size() != out.size())
                    throw std::runtime_error(std::string("Couldn't resolve requests"));

                return out;
            }

            std::map<index_type, request_type> _requests;
            bool require_all;
        };
    }
}
