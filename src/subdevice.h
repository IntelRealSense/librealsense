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

        void set_owner(const rs_streaming_lock* owner) { _owner = owner; }

        void play(frame_callback_ptr callback);

        virtual void stop();

        rs_frame* alloc_frame(size_t size, frame_additional_data additional_data) const;

        void invoke_callback(rs_frame* frame_ref) const;

        void flush() const;

        virtual ~streaming_lock();

        bool is_streaming() const { return _is_streaming; }

    private:
        std::atomic<bool> _is_streaming;
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
        std::shared_ptr<frame_archive> _archive;
        std::atomic<uint32_t> _max_publish_list_size;
        const rs_streaming_lock* _owner;
    };

    class endpoint
    {
    public:
        endpoint() : stream_profiles([this]() { return this->init_stream_profiles(); }) {}

        virtual std::vector<uvc::stream_profile> init_stream_profiles() = 0;
        std::vector<uvc::stream_profile> get_stream_profiles()
        {
            return *stream_profiles;
        }

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
        lazy<std::vector<uvc::stream_profile> > stream_profiles;
    };

    struct frame_timestamp_reader
    {
        virtual bool validate_frame(const request_mapping & mode, const void * frame) const = 0;
        virtual double get_frame_timestamp(const request_mapping& mode, const void * frame) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const void * frame) const = 0;
    };

    // TODO: This may need to be modified for thread safety
    class rolling_timestamp_reader : public frame_timestamp_reader
    {
        bool started;
        int64_t total;
        int last_timestamp;
        mutable int64_t counter = 0;
    public:
        rolling_timestamp_reader() : started(), total() {}

        bool validate_frame(const request_mapping& mode, const void * frame) const override
        {
            // Validate that at least one byte of the image is nonzero
            for (const uint8_t * it = (const uint8_t *)frame, *end = it + mode.pf->get_image_size(mode.profile.width, mode.profile.height); it != end; ++it)
            {
                if (*it)
                {
                    return true;
                }
            }

            // F200 and SR300 can sometimes produce empty frames shortly after starting, ignore them
            //LOG_INFO("Subdevice " << mode.subdevice << " produced empty frame");
            return false;
        }

        double get_frame_timestamp(const request_mapping& /*mode*/, const void * frame) override
        {
            // Timestamps are encoded within the first 32 bits of the image
            int rolling_timestamp = *reinterpret_cast<const int32_t *>(frame);

            if (!started)
            {
                last_timestamp = rolling_timestamp;
                started = true;
            }

            const int delta = rolling_timestamp - last_timestamp; // NOTE: Relies on undefined behavior: signed int wraparound
            last_timestamp = rolling_timestamp;
            total += delta;
            const int timestamp = static_cast<int>(total / 100000);
            return timestamp;
        }
        unsigned long long get_frame_counter(const request_mapping & /*mode*/, const void * /*frame*/) const override
        {
            return ++counter;
        }
    };

    class uvc_endpoint : public endpoint, public std::enable_shared_from_this<uvc_endpoint>
    {
    public:
        explicit uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device)
            : _device(std::move(uvc_device)) {}

        std::vector<uvc::stream_profile> init_stream_profiles() override;

        std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) override;

        void register_xu(uvc::extension_unit xu)
        {
            _xus.push_back(std::move(xu));
        }

        std::vector<std::shared_ptr<frame_timestamp_reader>> create_frame_timestamp_readers() const
        {
            auto the_reader = std::make_shared<rolling_timestamp_reader>(); // single shared timestamp reader for all subdevices
            return{ the_reader, the_reader };                               // clone the reference for color and depth
        }

        template<class T>
        auto invoke_powered(T action)
            -> decltype(action(*static_cast<uvc::uvc_device*>(nullptr)))
        {
            power on(shared_from_this());
            return action(*_device);
        }

        void stop_streaming();
    private:
        void acquire_power();

        void release_power();

        struct power
        {
            explicit power(std::weak_ptr<uvc_endpoint> owner)
                : _owner(owner)
            {
                auto strong = _owner.lock();
                if (strong) strong->acquire_power();
            }

            ~power()
            {
                auto strong = _owner.lock();
                if (strong) strong->release_power();
            }
        private:
            std::weak_ptr<uvc_endpoint> _owner;
        };

        class uvc_streaming_lock : public streaming_lock
        {
        public:
            explicit uvc_streaming_lock(std::weak_ptr<uvc_endpoint> owner)
                : _owner(owner), _power(owner)
            {
            }

            void stop() override
            {
                streaming_lock::stop();
                auto strong = _owner.lock();
                if (strong) strong->stop_streaming();
            }
        private:
            std::weak_ptr<uvc_endpoint> _owner;
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
