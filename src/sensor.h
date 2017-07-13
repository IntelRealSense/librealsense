// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"

#include "core/streaming.h"
#include "core/roi.h"
#include "core/options.h"
#include "source.h"

#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>
#include <limits.h>
#include <atomic>
#include <functional>

namespace librealsense
{
    class device;
    class option;

    typedef std::function<void(rs2_stream, rs2_frame&, callback_invocation_holder)> on_before_frame_callback;

    class options_container : public virtual options_interface
    {
    public:
        option& get_option(rs2_option id) override;
        const option& get_option(rs2_option id) const override;
        bool supports_option(rs2_option id) const override;

        void register_option(rs2_option id, std::shared_ptr<option> option);

    private:
        std::map<rs2_option, std::shared_ptr<option>> _options;
    };

    class info_container : public virtual info_interface
    {
    public:
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

        void register_info(rs2_camera_info info, const std::string& val);

    private:
        std::map<rs2_camera_info, std::string> _camera_info;
    };

    class sensor_base : public virtual sensor_interface, public options_container, public virtual info_container
    {
    public:
        explicit sensor_base(std::string name, std::shared_ptr<uvc::time_service> ts, device* owner);


        virtual std::vector<uvc::stream_profile> init_stream_profiles() = 0;
        const std::vector<uvc::stream_profile>& get_stream_profiles() const
        {
            return *_stream_profiles;
        }

        void register_notifications_callback(notifications_callback_ptr callback) override;
        std::shared_ptr<notifications_proccessor> get_notifications_proccessor();

        bool is_streaming() const
        {
            return _is_streaming;
        }

        void register_pixel_format(native_pixel_format pf)
        {
            _pixel_formats.push_back(pf);
        }

        virtual ~sensor_base() { _source.flush(); }

        void register_metadata(rs2_frame_metadata metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser);

        void set_pose(lazy<pose> p) { _pose = std::move(p); }
        pose get_pose() const { return *_pose; }

        void register_on_before_frame_callback(on_before_frame_callback callback)
        {
            _on_before_frame_callback = callback;
        }

    protected:

        bool try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const;

        std::vector<request_mapping> resolve_requests(std::vector<stream_profile> requests);

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::shared_ptr<uvc::time_service> _ts;
        std::shared_ptr<notifications_proccessor> _notifications_proccessor;
        on_before_frame_callback _on_before_frame_callback;
        std::shared_ptr<metadata_parser_map> _metadata_parsers = nullptr;

        frame_source _source;
        device* _owner_dev;

    private:
        std::vector<native_pixel_format> _pixel_formats;
        lazy<std::vector<uvc::stream_profile>> _stream_profiles;
        lazy<pose> _pose;
    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual double get_frame_timestamp(const request_mapping& mode, const uvc::frame_object& fo) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const uvc::frame_object& fo) const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const uvc::frame_object& fo) const = 0;
        virtual void reset() = 0;
    };

    class hid_sensor : public sensor_base, 
                       public std::enable_shared_from_this<hid_sensor>
    {
    public:
        explicit hid_sensor(std::shared_ptr<uvc::hid_device> hid_device,
                            std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
                            std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
                            std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
                            std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles,
                            std::shared_ptr<uvc::time_service> ts, device* owner)
            : sensor_base("Motion Module", ts, owner),_sensor_name_and_hid_profiles(sensor_name_and_hid_profiles),
              _fps_and_sampling_frequency_per_rs2_stream(fps_and_sampling_frequency_per_rs2_stream),
              _hid_device(hid_device),
              _is_configured_stream(RS2_STREAM_COUNT),
              _hid_iio_timestamp_reader(std::move(hid_iio_timestamp_reader)),
              _custom_hid_timestamp_reader(std::move(custom_hid_timestamp_reader))
        {
            std::map<std::string, uint32_t> frequency_per_sensor;
            for (auto& elem : sensor_name_and_hid_profiles)
                frequency_per_sensor.insert(std::make_pair(elem.first, elem.second.fps));

            std::vector<uvc::hid_profile> profiles_vector;
            for (auto& elem : frequency_per_sensor)
                profiles_vector.push_back(uvc::hid_profile{elem.first, elem.second});


            _hid_device->open(profiles_vector);
            for (auto& elem : _hid_device->get_sensors())
                _hid_sensors.push_back(elem);

            _hid_device->close();
        }

        ~hid_sensor();

        std::vector<stream_profile> get_principal_requests() override;

        void open(const std::vector<stream_profile>& requests) override;

        void close() override;

        void start(frame_callback_ptr callback);

        void stop();

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

    class uvc_sensor : public sensor_base, 
                       public roi_sensor_interface,
                       public std::enable_shared_from_this<uvc_sensor>
    {
    public:
        explicit uvc_sensor(std::string name, std::shared_ptr<uvc::uvc_device> uvc_device,
                              std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                              std::shared_ptr<uvc::time_service> ts, device* owner)
            : sensor_base(name, ts, owner),
              _device(std::move(uvc_device)),
              _user_count(0),
              _timestamp_reader(std::move(timestamp_reader))
        {}

        ~uvc_sensor();

        region_of_interest_method& get_roi_method() const
        {
            if (!_roi_method.get())
                throw librealsense::not_implemented_exception("Region-of-interest is not implemented for this device!");
            return *_roi_method;
        }
        void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method)
        {
            _roi_method = roi_method;
        }

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

        void start(frame_callback_ptr callback);

        void stop();

    private:
        std::vector<uvc::stream_profile> init_stream_profiles() override;

        void acquire_power();

        void release_power();

        void reset_streaming();

        struct power
        {
            explicit power(std::weak_ptr<uvc_sensor> owner)
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
            std::weak_ptr<uvc_sensor> _owner;
        };

        std::shared_ptr<uvc::uvc_device> _device;
        std::atomic<int> _user_count;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
        std::unique_ptr<power> _power;
        std::unique_ptr<frame_timestamp_reader> _timestamp_reader;
        std::shared_ptr<region_of_interest_method> _roi_method = nullptr;
    };
}
