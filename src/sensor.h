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
#include <core/debug.h>

namespace librealsense
{
    class device;
    class option;

    typedef std::function<void(rs2_stream, frame_interface*, callback_invocation_holder)> on_before_frame_callback;
    typedef std::function<void(std::vector<platform::stream_profile>)> on_open;

    class sensor_base : public std::enable_shared_from_this<sensor_base>,
                        public virtual sensor_interface, public options_container, public virtual info_container
    {
    public:
        explicit sensor_base(std::string name,
                             device* device);

        virtual stream_profiles init_stream_profiles() = 0;

        stream_profiles get_stream_profiles() const override
        {
            return *_profiles;
        }

        void register_notifications_callback(notifications_callback_ptr callback) override;
        std::shared_ptr<notifications_proccessor> get_notifications_proccessor();

        bool is_streaming() const override
        {
            return _is_streaming;
        }

        virtual ~sensor_base() { _source.flush(); }

        void register_metadata(rs2_frame_metadata_value metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser) const;

        void register_on_open(on_open callback)
        {
            _on_open = callback;
        }

        void register_on_before_frame_callback(on_before_frame_callback callback)
        {
            _on_before_frame_callback = callback;
        }

        const device_interface& get_device() override;

        void register_pixel_format(native_pixel_format pf)
        {
            _pixel_formats.push_back(pf);
        }

        // Make sensor inherit its owning device info by default
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

    protected:
        bool try_get_pf(const platform::stream_profile& p, native_pixel_format& result) const;

        void assign_stream(const std::shared_ptr<stream_interface>& stream,
                           std::shared_ptr<stream_profile_interface>& target) const;

        std::vector<request_mapping> resolve_requests(stream_profiles requests);

        std::vector<platform::stream_profile> _internal_config;

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::shared_ptr<notifications_proccessor> _notifications_proccessor;
        on_before_frame_callback _on_before_frame_callback;
        on_open _on_open;
        std::shared_ptr<metadata_parser_map> _metadata_parsers = nullptr;

        frame_source _source;
        device* _owner;

    private:
        lazy<stream_profiles> _profiles;
        std::vector<native_pixel_format> _pixel_formats;
    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual double get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const platform::frame_object& fo) const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const = 0;
        virtual void reset() = 0;
    };

    class hid_sensor : public sensor_base
    {
    public:
        explicit hid_sensor(std::shared_ptr<platform::hid_device> hid_device,
                            std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
                            std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
                            std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
                            std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles,
                            device* dev);

        ~hid_sensor();

        void open(const stream_profiles& requests) override;

        void close() override;

        void start(frame_callback_ptr callback) override;

        void stop() override;

        std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                    const std::string& report_name,
                                                    platform::custom_sensor_report_field report_field) const;

    protected:
        stream_profiles init_stream_profiles() override;

    private:
        const std::map<rs2_stream, uint32_t> stream_and_fourcc = {{RS2_STREAM_GYRO,  'GYRO'},
                                                                  {RS2_STREAM_ACCEL, 'ACCL'},
                                                                  {RS2_STREAM_GPIO,  'GPIO'}};

        const std::vector<std::pair<std::string, stream_profile>> _sensor_name_and_hid_profiles;
        std::map<rs2_stream, std::map<uint32_t, uint32_t>> _fps_and_sampling_frequency_per_rs2_stream;
        std::shared_ptr<platform::hid_device> _hid_device;
        std::mutex _configure_lock;
        std::map<std::string, stream_profile> _configured_profiles;
        std::vector<bool> _is_configured_stream;
        std::vector<platform::hid_sensor> _hid_sensors;
        std::map<std::string, request_mapping> _hid_mapping;
        std::unique_ptr<frame_timestamp_reader> _hid_iio_timestamp_reader;
        std::unique_ptr<frame_timestamp_reader> _custom_hid_timestamp_reader;

        stream_profiles get_sensor_profiles(std::string sensor_name) const;

        const std::string& rs2_stream_to_sensor_name(rs2_stream stream) const;

        uint32_t stream_to_fourcc(rs2_stream stream) const;

        uint32_t fps_to_sampling_frequency(rs2_stream stream, uint32_t fps) const;
    };

    class uvc_sensor : public sensor_base,
                       public roi_sensor_interface
    {
    public:
        explicit uvc_sensor(std::string name, std::shared_ptr<platform::uvc_device> uvc_device,
                            std::unique_ptr<frame_timestamp_reader> timestamp_reader, device* dev);

        ~uvc_sensor();

        region_of_interest_method& get_roi_method() const override;
        void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method) override;

        void open(const stream_profiles& requests) override;

        void close() override;

        std::vector<platform::stream_profile> get_configuration() const { return _internal_config; }

        void register_xu(platform::extension_unit xu);

        template<class T>
        auto invoke_powered(T action)
            -> decltype(action(*static_cast<platform::uvc_device*>(nullptr)))
        {
            power on(std::dynamic_pointer_cast<uvc_sensor>(shared_from_this()));
            return action(*_device);
        }

        void register_pu(rs2_option id);
        void try_register_pu(rs2_option id);

        void start(frame_callback_ptr callback) override;

        void stop() override;

    protected:
        stream_profiles init_stream_profiles() override;

        rs2_extension stream_to_frame_types(rs2_stream stream) const;

    private:
        void acquire_power();

        void release_power();

        void reset_streaming();

        struct power
        {
            explicit power(std::weak_ptr<uvc_sensor> owner)
                : _owner(owner)
            {
                auto strong = _owner.lock();
                if (strong)
                {
                    strong->acquire_power();
                }
            }

            ~power()
            {
                auto strong = _owner.lock();
                if (strong) strong->release_power();
            }
        private:
            std::weak_ptr<uvc_sensor> _owner;
        };

        std::shared_ptr<platform::uvc_device> _device;
        std::atomic<int> _user_count;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<platform::extension_unit> _xus;
        std::unique_ptr<power> _power;
        std::unique_ptr<frame_timestamp_reader> _timestamp_reader;
        std::shared_ptr<region_of_interest_method> _roi_method = nullptr;
    };
}
