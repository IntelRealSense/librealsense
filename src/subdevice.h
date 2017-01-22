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
#include <atomic>

namespace rsimpl
{
    class device;
    class option;

    class endpoint
    {
    public:
        endpoint()
            : _is_streaming(false),
              _is_opened(false),
              _callback(nullptr, [](rs_frame_callback*) {}),
              _max_publish_list_size(16),
              _stream_profiles([this]() { return this->init_stream_profiles(); }) {}

        virtual std::vector<uvc::stream_profile> init_stream_profiles() = 0;
        const std::vector<uvc::stream_profile>& get_stream_profiles() const
        {
            return *_stream_profiles;
        }

        virtual void start_streaming(frame_callback_ptr callback) = 0;

        virtual void stop_streaming() = 0;

        rs_frame* alloc_frame(size_t size, frame_additional_data additional_data) const;

        void invoke_callback(rs_frame* frame_ref) const;

        void flush() const;

        bool is_streaming() const
        {
            return _is_streaming;
        }

        virtual std::vector<stream_profile> get_principal_requests() = 0;
        virtual void open(const std::vector<stream_profile>& requests) = 0;
        virtual void close() = 0;

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
        void register_info(rs_camera_info info, const std::string& val);

        void set_pose(pose p) { _pose = std::move(p); }
        const pose& get_pose() const { return _pose; }

    protected:

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const;

        std::vector<request_mapping> resolve_requests(std::vector<stream_profile> requests);

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
        std::shared_ptr<frame_archive> _archive;
        std::atomic<uint32_t> _max_publish_list_size;

    private:

        std::map<rs_option, std::shared_ptr<option>> _options;
        std::vector<native_pixel_format> _pixel_formats;
        lazy<std::vector<uvc::stream_profile>> _stream_profiles;
        pose _pose;
        std::map<rs_camera_info, std::string> _camera_info;
    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual bool validate_frame(const request_mapping & mode, const void * frame) const = 0;
        virtual double get_frame_timestamp(const request_mapping& mode, const void * frame) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const void * frame) const = 0;
        virtual rs_timestamp_domain get_frame_timestamp_domain(const request_mapping& mode) const = 0;
        virtual void reset() = 0;
    };

    class hid_endpoint : public endpoint, public std::enable_shared_from_this<hid_endpoint>
    {
    public:
        explicit hid_endpoint(std::shared_ptr<uvc::hid_device> hid_device,
                              std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : _hid_device(hid_device),
              _timestamp_reader(std::move(timestamp_reader))
        {
            _hid_device->open();
            for (auto& elem : _hid_device->get_sensors())
                _hid_sensors.push_back(elem);

            _hid_device->close();
        }

        ~hid_endpoint();

        std::vector<stream_profile> get_principal_requests() override;

        void open(const std::vector<stream_profile>& requests) override;

        void close() override;

        void start_streaming(frame_callback_ptr callback);

        void stop_streaming();

    private:

        struct stream_formats{
            rs_stream stream;
            std::vector<rs_format> formats;
        };

        const std::map<rs_stream, uint32_t> stream_and_fourcc = {{RS_STREAM_GYRO, 'GYRO'},
                                                                 {RS_STREAM_ACCEL, 'ACCL'}};

        const std::map<std::string, stream_formats> sensor_name_and_stream_formats =
            {{std::string("gyro_3d"),  {RS_STREAM_GYRO,  {RS_FORMAT_MOTION_RAW, RS_FORMAT_MOTION_XYZ32F}}},
             {std::string("accel_3d"), {RS_STREAM_ACCEL, {RS_FORMAT_MOTION_RAW, RS_FORMAT_MOTION_XYZ32F}}}};

        std::shared_ptr<uvc::hid_device> _hid_device;
        std::mutex _configure_lock;
        std::map<int, stream_formats> _configured_profiles;
        std::vector<uvc::hid_sensor> _hid_sensors;
        std::map<int, request_mapping> _iio_mapping;
        std::unique_ptr<frame_timestamp_reader> _timestamp_reader;

        std::vector<uvc::stream_profile> init_stream_profiles() override;

        std::vector<stream_profile> get_device_profiles();

        int rs_stream_to_sensor_iio(rs_stream stream) const;

        int get_iio_by_name(const std::string& name) const;

        stream_formats sensor_name_to_stream_formats(const std::string& sensor_name) const;

        uint32_t stream_to_fourcc(rs_stream stream) const;
    };

    class uvc_endpoint : public endpoint, public std::enable_shared_from_this<uvc_endpoint>
    {
    public:
        explicit uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device,
                              std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : _device(std::move(uvc_device)),
              _user_count(0),
              _timestamp_reader(std::move(timestamp_reader))
        {}

        ~uvc_endpoint();

        std::vector<stream_profile> get_principal_requests() override;

        void open(const std::vector<stream_profile>& requests) override;

        void close() override;

        void register_xu(uvc::extension_unit xu);

        template<class T>
        auto invoke_powered(T action)
            -> decltype(action(*static_cast<uvc::uvc_device*>(nullptr)))
        {
            power on(shared_from_this());
            return action(*_device);
        }


        void register_pu(rs_option id);

        void start_streaming(frame_callback_ptr callback);

        void stop_streaming();
    private:
        std::vector<uvc::stream_profile> init_stream_profiles() override;

        void acquire_power();

        void release_power();

        void reset_streaming();

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

        std::shared_ptr<uvc::uvc_device> _device;
        std::atomic<int> _user_count;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
        std::unique_ptr<power> _power;
        std::unique_ptr<frame_timestamp_reader> _timestamp_reader;
    };
}
