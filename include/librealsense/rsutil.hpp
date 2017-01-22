/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RSUTIL_HPP
#define LIBREALSENSE_RSUTIL_HPP

#include "rs.hpp"
#include "rsutil.h"

#include <map>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>

namespace rs
{
    enum class preset
    {
        best_quality,
        largest_image,
        highest_framerate,
    };

    namespace util
    {
        template<class Dev = rs::device>
        class Config
        {
        public:
            class multistream
            {
            public:
                multistream() {}

                explicit multistream(std::vector<Dev> results,
                                     std::map<rs_stream, stream_profile> profiles,
                                     std::map<rs_stream, Dev> devices)
                    : results(std::move(results)),
                      profiles(std::move(profiles)),
                      devices(std::move(devices)) {}

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

                rs_intrinsics get_intrinsics(rs_stream stream) try
                {
                    return devices.at(stream).get_intrinsics(profiles.at(stream));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have intrinsics for ") + rs_stream_to_string(stream));
                }

                rs_extrinsics get_extrinsics(rs_stream from, rs_stream to) const try
                {
                    return devices.at(from).get_extrinsics_to(devices.at(to));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error(std::string("config doesnt have extrinsics for ") + rs_stream_to_string(from) + "->" + rs_stream_to_string(to));
                }

            private:
                std::vector<Dev> streams;
                std::map<rs_stream, stream_profile> profiles;
                std::map<rs_stream, Dev> devices;
                std::vector<Dev> results;
            };

            Config() {}

            void enable_stream(rs_stream stream, int width, int height, int fps, rs_format format)
            {
                _presets.erase(stream);
                _requests[stream] = { stream, width, height, fps, format };
            }
            void disable_stream(rs_stream stream)
            {
                _presets.erase(stream);
                _requests.erase(stream);
            }
            void enable_stream(rs_stream stream, preset preset)
            {
                _requests.erase(stream);
                _presets[stream] = preset;
            }
            void enable_all(preset p)
            {
                for (int i = RS_STREAM_DEPTH; i < RS_STREAM_COUNT; i++)
                {
                    enable_stream(static_cast<rs_stream>(i), p);
                }
            }

            void disable_all()
            {
                _presets.clear();
                _requests.clear();
            }

            void close(Dev dev)
            {
                dev.close();
            }

            multistream open(Dev dev)
            {
                std::vector<Dev> results;
                std::vector<rs_stream> satisfied_streams;
                std::map<rs_stream, Dev> devices;
                std::map<rs_stream, stream_profile> stream_to_profile;

                for (auto&& sub : dev.get_adjacent_devices())
                {
                    std::vector<stream_profile> targets;
                    auto profiles = sub.get_stream_modes();

                    // deal with explicit requests
                    for (auto&& kvp : _requests)
                    {
                        if (find(begin(satisfied_streams),
                            end(satisfied_streams), kvp.first)
                            != end(satisfied_streams)) continue; // skip satisfied requests

                        // if any profile on the subdevice can supply this request, consider it satisfiable
                        if (std::any_of(begin(profiles), end(profiles),
                            [&kvp](const stream_profile& profile)
                            {
                                return kvp.first == profile.stream;
                            }))
                        {
                            targets.push_back(kvp.second); // store that this request is going to this subdevice
                            satisfied_streams.push_back(kvp.first); // mark stream as satisfied
                        }
                    }

                    // deal with preset streams
                    std::vector<rs_format> prefered_formats = { RS_FORMAT_Z16, RS_FORMAT_Y8, RS_FORMAT_RGB8 };
                    for (auto&& kvp : _presets)
                    {
                        if (find(begin(satisfied_streams),
                            end(satisfied_streams), kvp.first)
                            != end(satisfied_streams)) continue; // skip if already satisified

                        stream_profile result = { RS_STREAM_COUNT, 0, 0, 0, RS_FORMAT_ANY };
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
                        default: throw std::runtime_error("unknown preset selected");
                        }

                        for (auto itr = profiles.rbegin(); itr != profiles.rend(); ++itr) {
                            if (itr->stream == kvp.first) {
                                result = *itr;
                                break;
                            }
                        }

                        // RS_STREAM_COUNT signals subdevice can't handle this stream
                        if (result.stream != RS_STREAM_COUNT)
                        {
                            targets.push_back(result);
                            satisfied_streams.push_back(kvp.first);
                            stream_to_profile[result.stream] = result;
                        }
                    }

                    if (targets.size() > 0) // if subdevice is handling any streams
                    {
                        auto_complete(targets, sub);

                        for (auto&& t : targets)
                        {
                            stream_to_profile[t.stream] = t;
                        }

                        try
                        {
                            sub.open(targets); // TODO: RAII device streaming
                        }
                        catch(...)
                        {
                            for (auto&& elem : results)
                                elem.close();

                            results.clear();
                            throw;
                        }

                        for (auto && target : targets)
                        {
                            devices.emplace(std::make_pair(target.stream, sub));
                        }
                        results.push_back(std::move(sub));
                    }
                }

                return multistream( move(results), move(stream_to_profile), move(devices) );
            }

        private:
            static bool sort_highest_framerate(const stream_profile& lhs, const stream_profile &rhs) {
                return lhs.fps < rhs.fps;
            }

            static bool sort_largest_image(const stream_profile& lhs, const stream_profile &rhs) {
                return lhs.width*lhs.height < rhs.width*rhs.height;
            }

            static bool sort_best_quality(const stream_profile& lhs, const stream_profile& rhs) {
                return std::make_tuple((lhs.width == 640 && lhs.height == 480), (lhs.fps == 30), (lhs.format == RS_FORMAT_Y8), (lhs.format == RS_FORMAT_RGB8), int(lhs.format))
                     < std::make_tuple((rhs.width == 640 && rhs.height == 480), (rhs.fps == 30), (rhs.format == RS_FORMAT_Y8), (rhs.format == RS_FORMAT_RGB8), int(rhs.format));
                    

            }

            static void auto_complete(std::vector<stream_profile> &requests, Dev &target)
            {
                auto candidates = target.get_stream_modes();
                for (auto & request : requests)
                {
                    if (!request.has_wildcards()) continue;
                    for (auto candidate : candidates)
                    {
                        if (candidate.match(request) && !candidate.contradicts(requests))
                        {
                            request = candidate;
                            break;
                        }
                    }
                    if (request.has_wildcards())
                        throw std::runtime_error(std::string("Couldn't autocomplete request for subdevice ") + target.get_camera_info(RS_CAMERA_INFO_MODULE_NAME));
                }
            }

            std::map<rs_stream, stream_profile> _requests;
            std::map<rs_stream, preset> _presets;
        };

        typedef Config<> config;

        typedef std::vector<frame> frameset;

        class syncer
        {
        public:
            explicit syncer(rs_stream key_stream = RS_STREAM_DEPTH) 
                : impl(new shared_impl())
            {
                impl->key_stream = key_stream;
            }

            void set_key_stream(rs_stream stream)
            {
                impl->key_stream = stream;
            }

            void operator()(frame f)
            {
                using namespace std;

                unique_lock<recursive_mutex> lock(impl->mutex);
                auto stream_type = f.get_stream_type();
                impl->streams[stream_type].queue.enqueue(move(f));

                lock.unlock();
                if (stream_type == impl->key_stream) impl->cv.notify_one();
            }

            frameset wait_for_frames(int timeout_ms = 5000)
            {
                using namespace std;
                unique_lock<recursive_mutex> lock(impl->mutex);
                const auto ready = [this]()
                {
                    return impl->streams[impl->key_stream].queue.poll_for_frame(
                        &impl->streams[impl->key_stream].front);
                };

                frameset result;

                if (!ready())
                {
                    if (!impl->cv.wait_for(lock, chrono::milliseconds(timeout_ms), ready))
                    {
                        return result;
                    }
                }

                
                get_frameset(&result);
                return result;
            }

            bool poll_for_frames(frameset& frames)
            {
                using namespace std;
                unique_lock<recursive_mutex> lock(impl->mutex);
                if (!impl->streams[impl->key_stream].queue.poll_for_frame(
                        &impl->streams[impl->key_stream].front)) return false;
                get_frameset(&frames);
                return true;
            }

        private:
            struct stream_pipe
            {
                stream_pipe() : queue(4) {}
                frame front;
                frame_queue queue;

                ~stream_pipe()
                {
                    queue.flush();
                }
            };

            struct shared_impl
            {
                std::recursive_mutex mutex;
                std::condition_variable_any cv;
                rs_stream key_stream;
                stream_pipe streams[RS_STREAM_COUNT];
            };

            std::shared_ptr<shared_impl> impl;

            double dist(const frame& a, const frame& b) const
            {
                return std::fabs(a.get_timestamp() - b.get_timestamp());
            }

            void get_frameset(frameset* frames)
            {
                frames->clear();
                for (auto i = 0; i < RS_STREAM_COUNT; i++)
                {
                    if (i == impl->key_stream) continue;

                    frame res;
                    while ((!impl->streams[i].front || impl->streams[i].front.try_clone_ref(&res)) &&
                           impl->streams[i].queue.poll_for_frame(&impl->streams[i].front) &&
                           impl->streams[i].front && res &&
                           dist(impl->streams[i].front, impl->streams[impl->key_stream].front) <
                           dist(res, impl->streams[impl->key_stream].front)) { }
                    if (res) frames->push_back(std::move(res));
                }
                frames->push_back(std::move(impl->streams[impl->key_stream].front));
            }
        };
    }
}

#endif
