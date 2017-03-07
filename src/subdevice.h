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
#include <functional>

namespace rsimpl2
{
    class device;
    class option;

    typedef std::function<void(rs2_stream, rs2_frame&, callback_invocation_holder)> on_before_frame_callback;

    struct region_of_interest
    {
        int min_x;
        int min_y;
        int max_x;
        int max_y;
    };

    class region_of_interest_method
    {
    public:
        virtual void set(const region_of_interest& roi) = 0;
        virtual region_of_interest get() const = 0;

        virtual ~region_of_interest_method() = default;
    };

    class endpoint
    {
    public:
        endpoint(std::shared_ptr<uvc::time_service> ts)
            : _is_streaming(false),
              _is_opened(false),
              _callback(nullptr, [](rs2_frame_callback*) {}),
              _max_publish_list_size(16),
              _stream_profiles([this]() { return this->init_stream_profiles(); }),
              _ts(ts)
              {}

        virtual std::vector<uvc::stream_profile> init_stream_profiles() = 0;
        const std::vector<uvc::stream_profile>& get_stream_profiles() const
        {
            return *_stream_profiles;
        }

        virtual void start_streaming(frame_callback_ptr callback) = 0;

        virtual void stop_streaming() = 0;

        rs2_frame* alloc_frame(size_t size, frame_additional_data additional_data) const;

        void invoke_callback(frame_holder frame) const;

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

        option& get_option(rs2_option id);
        const option& get_option(rs2_option id) const;
        void register_option(rs2_option id, std::shared_ptr<option> option);
        bool supports_option(rs2_option id) const;

        const std::string& get_info(rs2_camera_info info) const;
        bool supports_info(rs2_camera_info info) const;
        void register_info(rs2_camera_info info, const std::string& val);

        void set_pose(pose p) { _pose = std::move(p); }
        const pose& get_pose() const { return _pose; }

        region_of_interest_method& get_roi_method() const
        {
            if (!_roi_method.get())
                throw rsimpl2::not_implemented_exception("Region-of-interest is not implemented for this device!");
            return *_roi_method;
        }
        void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method)
        {
            _roi_method = roi_method;
        }

    protected:

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const;

        std::vector<request_mapping> resolve_requests(std::vector<stream_profile> requests);

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
        std::shared_ptr<frame_archive> _archive;
        std::atomic<uint32_t> _max_publish_list_size;
        std::shared_ptr<uvc::time_service> _ts;

    private:

        std::map<rs2_option, std::shared_ptr<option>> _options;
        std::vector<native_pixel_format> _pixel_formats;
        lazy<std::vector<uvc::stream_profile>> _stream_profiles;
        pose _pose;
        std::map<rs2_camera_info, std::string> _camera_info;
        std::shared_ptr<region_of_interest_method> _roi_method = nullptr;

    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual double get_frame_timestamp(const request_mapping& mode, const uvc::frame_object& fo) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const uvc::frame_object& fo) const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const uvc::frame_object& fo) const = 0;
        virtual void reset() = 0;
    };

    class hid_endpoint : public endpoint, public std::enable_shared_from_this<hid_endpoint>
    {
    public:
        explicit hid_endpoint(std::shared_ptr<uvc::hid_device> hid_device,
                              std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                              std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
                              std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles,
                              std::shared_ptr<uvc::time_service> ts)
            : endpoint(ts),_hid_device(hid_device),
              _timestamp_reader(std::move(timestamp_reader)),
              _fps_and_sampling_frequency_per_rs2_stream(fps_and_sampling_frequency_per_rs2_stream),
              _sensor_name_and_hid_profiles(sensor_name_and_hid_profiles)
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

        const std::map<rs2_stream, uint32_t> stream_and_fourcc = {{RS2_STREAM_GYRO,  'GYRO'},
                                                                 {RS2_STREAM_ACCEL, 'ACCL'}};

        const std::vector<std::pair<std::string, stream_profile>> _sensor_name_and_hid_profiles;
        std::map<rs2_stream, std::map<uint32_t, uint32_t>> _fps_and_sampling_frequency_per_rs2_stream;
        std::shared_ptr<uvc::hid_device> _hid_device;
        std::mutex _configure_lock;
        std::map<uint32_t, stream_profile> _configured_profiles;
        std::vector<uvc::hid_sensor> _hid_sensors;
        std::map<uint32_t, request_mapping> _iio_mapping;
        std::unique_ptr<frame_timestamp_reader> _timestamp_reader;

        std::vector<stream_profile> get_sensor_profiles(std::string sensor_name) const;

        std::vector<uvc::stream_profile> init_stream_profiles() override;

        std::vector<stream_profile> get_device_profiles();

        uint32_t rs2_stream_to_sensor_iio(rs2_stream stream) const;

        uint32_t get_iio_by_name(const std::string& name) const;

        uint32_t stream_to_fourcc(rs2_stream stream) const;

        uint32_t fps_to_sampling_frequency(rs2_stream stream, uint32_t fps) const;
    };

    class uvc_endpoint : public endpoint, public std::enable_shared_from_this<uvc_endpoint>
    {
    public:
        explicit uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device,
                              std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                              std::shared_ptr<uvc::time_service> ts)
            : endpoint(ts),
              _device(std::move(uvc_device)),
              _user_count(0),
              _timestamp_reader(std::move(timestamp_reader)),
              _on_before_frame_callback(nullptr)
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

        void register_pu(rs2_option id);

        void register_on_before_frame_callback(on_before_frame_callback callback)
        {
            _on_before_frame_callback = callback;
        }


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
        on_before_frame_callback _on_before_frame_callback;
    };
}
