// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>

namespace rsimpl
{
    class device;

    class streaming_lock
    {
    public:
        streaming_lock();

        void set_owner(const rs_stream_lock* owner) { _owner = owner; }

        void play(frame_callback_ptr callback);

        void stop();

        void release_frame(rs_frame_ref* frame);

        rs_frame_ref* alloc_frame(size_t size, frame_additional_data additional_data);

        void invoke_callback(rs_frame_ref* frame_ref) const;

        void flush();

        virtual ~streaming_lock();

        bool is_streaming() const { return _is_streaming; }

    private:
        std::atomic<bool> _is_streaming;
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

        std::vector<stream_request> get_principal_requests();

        virtual std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) = 0;

        void register_pixel_format(native_pixel_format pf)
        {
            _pixel_formats.push_back(pf);
        }

        virtual ~endpoint() = default;

    protected:

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const;

        std::vector<request_mapping> resolve_requests(std::vector<stream_request> requests);

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
        void acquire_power();

        void release_power();

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
        int _user_count = 0;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
    };
}
