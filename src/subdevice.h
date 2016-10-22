// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "stream.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>
#include "hw-monitor.h"

namespace rsimpl
{
    class device;

    class streaming_lock
    {
    public:
        streaming_lock()
            : _callback(nullptr, [](rs_frame_callback*){}), 
              _archive(&max_publish_list_size), 
              _owner(nullptr)
        {
            
        }

        void set_owner(const rs_stream_lock* owner) { _owner = owner; }

        void play(frame_callback_ptr callback)
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback = std::move(callback);
        }

        void stop()
        {
            flush();
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback.reset();
        }

        void release_frame(rs_frame_ref* frame)
        {
            _archive.release_frame_ref(frame);
        }

        rs_frame_ref* alloc_frame(size_t size, frame_additional_data additional_data)
        {
            auto frame = _archive.alloc_frame(size, additional_data, true);
            return _archive.track_frame(frame);
        }

        void invoke_callback(rs_frame_ref* frame_ref) const
        {
            if (frame_ref)
            {
                frame_ref->update_frame_callback_start_ts(std::chrono::high_resolution_clock::now());
                //frame_ref->log_callback_start(capture_start_time);
                //on_before_callback(streams[i], frame_ref, archive);
                _callback->on_frame(_owner, frame_ref);
            }
        }

        void flush()
        {
            _archive.flush();
        }

        virtual ~streaming_lock()
        {
            stop();
        }

    private:
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
        frame_archive _archive;
        std::atomic<uint32_t> max_publish_list_size;
        const rs_stream_lock* _owner;
    };

    class endpoint
    {
    public:
        virtual std::vector<uvc::stream_profile> get_stream_profiles() = 0;


        std::vector<stream_request> get_principal_requests()
        {
            std::unordered_set<stream_request> results;

            auto profiles = get_stream_profiles();
            for (auto&& p : profiles)
            {
                native_pixel_format pf;
                if (try_get_pf(p, pf))
                {
                    for (auto&& unpacker : pf.unpackers)
                    {
                        for (auto&& output : unpacker.outputs)
                        {
                            results.insert({ output.first, p.width, p.height, p.fps, output.second });
                        }
                    }
                }
                else
                {
                    LOG_WARNING("Unsupported pixel-format " << p.format);
                }
            }

            std::vector<stream_request> res{ begin(results), end(results) };
            std::sort(res.begin(), res.end(), [](const stream_request& a, const stream_request& b)
            {
                return a.width > b.width;
            });
            return res;
        }

        virtual std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) = 0;

        void register_pixel_format(native_pixel_format pf)
        {
            _pixel_formats.push_back(pf);
        }

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const
        {
            auto it = std::find_if(begin(_pixel_formats), end(_pixel_formats), 
                [&p](const native_pixel_format& pf)
            {
                return pf.fourcc == p.format;
            });
            if (it != end(_pixel_formats))
            {
                result = *it;
                return true;
            }
            return false;
        }

        std::vector<request_mapping> resolve_requests(std::vector<stream_request> requests)
        {
            std::unordered_set<request_mapping> results;

            while (!requests.empty() && !_pixel_formats.empty())
            {
                auto max = 0;
                auto max_elem = &_pixel_formats[0];
                for (auto&& pf : _pixel_formats)
                {
                    auto count = std::count_if(begin(requests), end(requests), 
                        [&pf](const stream_request& r) { return pf.satisfies(r); });
                    if (count > max && pf.plane_count == count)
                    {
                        max = count;
                        max_elem = &pf;
                    }
                }

                if (max == 0) break;

                requests.erase(std::remove_if(begin(requests), end(requests),
                    [max_elem, &results](const stream_request& r)
                {
                    if (max_elem->satisfies(r))
                    {
                        request_mapping mapping;
                        mapping.request = r;
                        mapping.profile = { r.width, r.height, r.fps, max_elem->fourcc };
                        mapping.unpacker = max_elem->find_unpacker(r);
                        mapping.pf = max_elem;

                        results.insert(mapping);
                        return true;
                    }
                    return false;
                }));
            }

            if (requests.empty()) return{ begin(results), end(results) };

            throw std::runtime_error("Subdevice unable to satisfy stream requests!");
        }

        virtual ~endpoint() = default;

    private:
        std::vector<native_pixel_format> _pixel_formats;
    };

    class uvc_endpoint : public endpoint
    {
    public:
        explicit uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device)
            : _device(std::move(uvc_device)) {}

        std::vector<uvc::stream_profile> get_stream_profiles() override;

        std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) override;

        void register_xu(uvc::extension_unit xu)
        {
            _xus.push_back(std::move(xu));
        }

        template<class T>
        auto invoke_powered(T action) 
            -> decltype(action(*static_cast<uvc::uvc_device*>(nullptr)))
        {
            power on(this);
            return action(*_device);
        }

        void stop_streaming();
    private:
        void acquire_power()
        {
            //std::lock_guard<std::mutex> lock(_power_lock);
            if (!_user_count) 
            {
                _device->set_power_state(uvc::D0);
                for (auto& xu : _xus) _device->init_xu(xu);
            }
            _user_count++;
        }
        void release_power()
        {
            //std::lock_guard<std::mutex> lock(_power_lock);
            _user_count--;
            if (!_user_count) _device->set_power_state(uvc::D3);
        }

        struct power
        {
            explicit power(uvc_endpoint* owner)
                : _owner(owner)
            {
                _owner->acquire_power();
            }

            ~power()
            {
                _owner->release_power();
            }
        private:
            uvc_endpoint* _owner;
        };

        class uvc_streaming_lock : public rsimpl::streaming_lock
        {
        public:
            explicit uvc_streaming_lock(uvc_endpoint* owner)
                : _owner(owner), _power(owner)
            {
            }

            const uvc_endpoint& get_endpoint() const { return *_owner; }
        private:
            uvc_endpoint* _owner;
            power _power;
        };

        std::shared_ptr<uvc::uvc_device> _device;
        //device* _owner;
        int _user_count = 0;
        //std::mutex _power_lock;
        //std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
    };
}
