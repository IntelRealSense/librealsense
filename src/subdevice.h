// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"

#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>
#include <limits.h>

namespace rsimpl
{
    class device;
    class option;

    class streaming_lock
    {
    public:
        streaming_lock();

        void set_owner(const rs_streaming_lock* owner) { _owner = owner; }

        virtual void play(frame_callback_ptr callback) = 0;

        virtual void stop() = 0;

        rs_frame* alloc_frame(size_t size, frame_additional_data additional_data) const;

        void invoke_callback(rs_frame* frame_ref) const;

        void flush() const;

        virtual ~streaming_lock() {}

        bool is_streaming() const { return _is_streaming; }

    private:
        std::shared_ptr<frame_archive> _archive;
        std::atomic<uint32_t> _max_publish_list_size;
        const rs_streaming_lock* _owner;

    protected:
        std::atomic<bool> _is_streaming;
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
    };

    class endpoint
    {
    public:
        virtual std::vector<uvc::stream_profile> get_stream_profiles() = 0;

        virtual std::vector<stream_request> get_principal_requests() = 0;

        virtual std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) = 0;

        void register_pixel_format(native_pixel_format pf)
        {
            _pixel_formats.push_back(pf);
        }

        virtual ~endpoint() = default;

        option& get_option(rs_option id);
        const option& get_option(rs_option id) const;
        void register_option(rs_option id, std::shared_ptr<option> option);
        bool supports_option(rs_option id) const;

        const std::string& get_info(rs_camera_info info) const;
        bool supports_info(rs_camera_info info) const;
        void register_info(rs_camera_info info, std::string val);

        void set_pose(pose p) { _pose = std::move(p); }
        const pose& get_pose() const { return _pose; }

    protected:

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const;

        std::vector<request_mapping> resolve_requests(std::vector<stream_request> requests);

    private:

        bool auto_complete_request(std::vector<stream_request>& requests);

        std::map<rs_option, std::shared_ptr<option>> _options;
        std::vector<native_pixel_format> _pixel_formats;
        pose _pose;
        std::map<rs_camera_info, std::string> _camera_info;
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

    class hid_endpoint : public endpoint, public std::enable_shared_from_this<hid_endpoint>
    {
    public:
        explicit hid_endpoint(std::shared_ptr<uvc::hid_device> hid_device)
            : _hid_device(hid_device)
        {}

        std::vector<uvc::stream_profile> get_stream_profiles() override
        {
            std::vector<uvc::stream_profile> sp;
            for (auto& elem : get_stream_requests())
            {
                sp.push_back(uvc::stream_profile{elem.width, elem.height, elem.fps, elem.format});
            }

            return sp;
        }

        std::vector<stream_request> get_principal_requests() override
        {
            return get_stream_requests();
        }

        std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_request>& requests) override
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            for (auto& elem : requests)
            {
                _sensor_iio.push_back(rs_stream_to_sensor_iio(elem.stream));
            }

            std::shared_ptr<hid_streaming_lock> streaming(new hid_streaming_lock(shared_from_this()));
            return std::move(streaming);
        }

        void start_streaming(streaming_lock* stream) // TODO: pass streaming_lock obj by weak_ptr
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            _hid_device->start_capture(_sensor_iio, [stream, this](const uvc::callback_data& data){
                if (!stream->is_streaming())
                    return;

                frame_additional_data additional_data;
                auto stream_format = sensor_name_to_stream_format(data.sensor.name);
                additional_data.format = stream_format.format;
                additional_data.stream_type = stream_format.stream;
                auto data_size = sizeof(data.value);
                additional_data.width = data_size;
                additional_data.height = 1;
                auto frame = stream->alloc_frame(data_size, additional_data);

                std::vector<byte> raw_data;
                for (auto i = 0 ; i < sizeof(data.value); ++i)
                {
                    raw_data.push_back((data.value >> (CHAR_BIT * i)) & 0xff);
                }

                memcpy(frame->get()->data.data(), raw_data.data(), data_size);
                stream->invoke_callback(frame);
            });
        }

        void stop_streaming()
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            _hid_device->stop_capture();
            _sensor_iio.clear();
        }

        class hid_streaming_lock : public streaming_lock
        {
        public:
            explicit hid_streaming_lock(std::weak_ptr<hid_endpoint> owner)
                : _owner(owner)
            {
            }

            void play(frame_callback_ptr callback) override
            {
                std::lock_guard<std::mutex> lock(_callback_mutex);
                _callback = std::move(callback);
                _is_streaming = true;
                auto strong = _owner.lock();
                if (strong) strong->start_streaming(this);
            }

            void stop() override
            {
                std::lock_guard<std::mutex> lock(_callback_mutex);
                auto strong = _owner.lock();
                if (strong) strong->stop_streaming();
                flush();
                _callback.reset();
                _is_streaming = false;
            }
        private:
            std::weak_ptr<hid_endpoint> _owner;
        };

    private:

        struct stream_format{
            rs_stream stream;
            rs_format format;
        };

        const std::map<std::string, stream_format> sensor_name_and_stream_format =
            {{std::string("gyro_3d"), {RS_STREAM_GYRO, RS_FORMAT_MOTION_DATA}},
             {std::string("accel_3d"), {RS_STREAM_ACCEL, RS_FORMAT_MOTION_DATA}}};

        std::shared_ptr<uvc::hid_device> _hid_device;
        std::mutex _configure_lock;
        std::vector<int> _sensor_iio;

        std::vector<stream_request> get_stream_requests()
        {
            std::vector<stream_request> stream_requests;
            for (auto& elem : _hid_device->get_sensors())
            {
                auto stream_format = sensor_name_to_stream_format(elem.name);
                stream_requests.push_back({stream_format.stream,
                                           0,
                                           0,
                                           0,
                                           stream_format.format});
            }

            return stream_requests;
        }

        int rs_stream_to_sensor_iio(rs_stream stream)
        {
            for (auto& elem : sensor_name_and_stream_format)
            {
                if (stream == elem.second.stream)
                    return get_iio_by_name(elem.first);
            }
            throw std::runtime_error("rs_stream not found!");
        }

        int get_iio_by_name(const std::string& name) const
        {
            auto sensors =_hid_device->get_sensors();
            for (auto& elem : sensors)
            {
                if (!elem.name.compare(name))
                    return elem.iio;
            }
            throw std::runtime_error("sensor_name not found!");
        }

        stream_format sensor_name_to_stream_format(const std::string& sensor_name) const
        {
            stream_format stream_and_format;
            try{
                stream_and_format = sensor_name_and_stream_format.at(sensor_name);
            }
            catch(...)
            {
                LOG_ERROR("format not found!");
                throw;
            }

            return stream_and_format;
        }
    };

    class uvc_endpoint : public endpoint, public std::enable_shared_from_this<uvc_endpoint>
    {
    public:
        explicit uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device)
            : _device(std::move(uvc_device)) {}

        std::vector<uvc::stream_profile> get_stream_profiles() override;

        std::vector<stream_request> get_principal_requests() override;

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


        void register_pu(rs_option id);


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

            void play(frame_callback_ptr callback) override
            {
                std::lock_guard<std::mutex> lock(_callback_mutex);
                _callback = std::move(callback);
                _is_streaming = true;
            }

            void stop() override
            {
                _is_streaming = false;
                flush();
                std::lock_guard<std::mutex> lock(_callback_mutex);
                _callback.reset();
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
