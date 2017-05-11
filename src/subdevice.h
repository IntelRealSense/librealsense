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

    class endpoint;

    class start_adaptor
    {
    public:
        explicit start_adaptor(endpoint* endpoint)
            : _set_callback_mutex(),
              _owner(endpoint)
        {
        }

        void start(rs2_stream stream, frame_callback_ptr ptr);
        void stop(rs2_stream stream);

        class frame_callback : public rs2_frame_callback
        {
        public:
            explicit frame_callback(start_adaptor* owner) 
                : _owner(owner) {}

            void on_frame(rs2_frame* f) override;

            void release() override {}
        private:
            start_adaptor* _owner;
        };

    private:
        friend class frame_callback;
        std::mutex _set_callback_mutex;
        std::map<rs2_stream, frame_callback_ptr> _callbacks;
        uint32_t _active_callbacks = 0;
        endpoint* const _owner;
    };

    class endpoint
    {
    public:
        explicit endpoint(std::shared_ptr<uvc::time_service> ts);

        virtual std::vector<uvc::stream_profile> init_stream_profiles() = 0;
        const std::vector<uvc::stream_profile>& get_stream_profiles() const
        {
            return *_stream_profiles;
        }

        virtual void start_streaming(frame_callback_ptr callback) = 0;

        virtual void stop_streaming() = 0;

        void register_notifications_callback(notifications_callback_ptr callback);

        rs2_frame* alloc_frame(size_t size, frame_additional_data additional_data, bool requires_memory) const;

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

        void set_pose(lazy<pose> p) { _pose = std::move(p); }
        pose get_pose() const { return *_pose; }

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
        std::shared_ptr<notifications_proccessor> get_notifications_proccessor();

        start_adaptor& starter() { return _start_adaptor; }

        void register_on_before_frame_callback(on_before_frame_callback callback)
        {
            _on_before_frame_callback = callback;
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
        std::shared_ptr<notifications_proccessor> _notifications_proccessor;
        on_before_frame_callback _on_before_frame_callback;

    private:
        std::map<rs2_option, std::shared_ptr<option>> _options;
        std::vector<native_pixel_format> _pixel_formats;
        lazy<std::vector<uvc::stream_profile>> _stream_profiles;
        lazy<pose> _pose;
        std::map<rs2_camera_info, std::string> _camera_info;
        std::shared_ptr<region_of_interest_method> _roi_method = nullptr;
        start_adaptor _start_adaptor;
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
                              std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
                              std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
                              std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
                              std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles,
                              std::shared_ptr<uvc::time_service> ts)
            : endpoint(ts),_hid_device(hid_device),
              _hid_iio_timestamp_reader(std::move(hid_iio_timestamp_reader)),
              _custom_hid_timestamp_reader(std::move(custom_hid_timestamp_reader)),
              _fps_and_sampling_frequency_per_rs2_stream(fps_and_sampling_frequency_per_rs2_stream),
              _sensor_name_and_hid_profiles(sensor_name_and_hid_profiles),
              _is_configured_stream(RS2_STREAM_COUNT)
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

        std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                    const std::string& report_name,
                                                    uvc::custom_sensor_report_field report_field)
        {
            return _hid_device->get_custom_report_data(custom_sensor_name, report_name, report_field);
        }

    private:

        const std::map<rs2_stream, uint32_t> stream_and_fourcc = {{RS2_STREAM_GYRO,  'GYRO'},
                                                                  {RS2_STREAM_ACCEL, 'ACCL'},
                                                                  {RS2_STREAM_GPIO1, 'GPIO'},
                                                                  {RS2_STREAM_GPIO2, 'GPIO'},
                                                                  {RS2_STREAM_GPIO3, 'GPIO'},
                                                                  {RS2_STREAM_GPIO4, 'GPIO'}};

        const std::vector<std::pair<std::string, stream_profile>> _sensor_name_and_hid_profiles;
        std::map<rs2_stream, std::map<uint32_t, uint32_t>> _fps_and_sampling_frequency_per_rs2_stream;
        std::shared_ptr<uvc::hid_device> _hid_device;
        std::mutex _configure_lock;
        std::map<std::string, stream_profile> _configured_profiles;
        std::vector<bool> _is_configured_stream;
        std::vector<uvc::hid_sensor> _hid_sensors;
        std::map<std::string, request_mapping> _hid_mapping;
        std::unique_ptr<frame_timestamp_reader> _hid_iio_timestamp_reader;
        std::unique_ptr<frame_timestamp_reader> _custom_hid_timestamp_reader;

        std::vector<stream_profile> get_sensor_profiles(std::string sensor_name) const;

        std::vector<uvc::stream_profile> init_stream_profiles() override;

        std::vector<stream_profile> get_device_profiles();

        const std::string& rs2_stream_to_sensor_name(rs2_stream stream) const;

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

        void register_pu(rs2_option id);

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
