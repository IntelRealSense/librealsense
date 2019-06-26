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

template<typename T>
uint32_t rs_fourcc(const T a, const T b, const  T c, const T d)
{
    static_assert((std::is_integral<T>::value), "rs_fourcc supports integral built-in types only");
    return ((static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8) |
            (static_cast<uint32_t>(d) << 0));
}

namespace librealsense
{
    class device;
    class option;

    typedef std::function<void(rs2_stream, frame_interface*, callback_invocation_holder)> on_before_frame_callback;
    typedef std::function<void(std::vector<platform::stream_profile>)> on_open;

    class sensor_base : public std::enable_shared_from_this<sensor_base>,
                        public virtual sensor_interface, public options_container, public virtual info_container, public recommended_proccesing_blocks_base
    {
    public:
        explicit sensor_base(std::string name,
                             device* device, 
                             recommended_proccesing_blocks_interface* owner);
        virtual ~sensor_base() override { _source.flush(); }

        virtual stream_profiles init_stream_profiles() = 0;

        stream_profiles get_stream_profiles(int tag = profile_tag::PROFILE_TAG_ANY) const override;

        virtual stream_profiles get_active_streams() const override;
        notifications_callback_ptr get_notifications_callback() const override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        int register_before_streaming_changes_callback(std::function<void(bool)> callback) override;
        void unregister_before_start_callback(int token) override;
        std::shared_ptr<notifications_processor> get_notifications_processor();
        virtual frame_callback_ptr get_frames_callback() const override;
        virtual void set_frames_callback(frame_callback_ptr callback) override;

        bool is_streaming() const override
        {
            return _is_streaming;
        }

        void register_metadata(rs2_frame_metadata_value metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser) const;

        void register_on_open(on_open callback)
        {
            _on_open = callback;
        }

        void register_on_before_frame_callback(on_before_frame_callback callback)
        {
            _on_before_frame_callback = callback;
        }

        device_interface& get_device() override;

        void register_pixel_format(native_pixel_format pf);
        void remove_pixel_format(native_pixel_format pf);

        // Make sensor inherit its owning device info by default
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

        processing_blocks get_recommended_processing_blocks() const override
        {
            return {};
        }
    protected:
        void raise_on_before_streaming_changes(bool streaming);
        void set_active_streams(const stream_profiles& requests);
        bool try_get_pf(const platform::stream_profile& p, native_pixel_format& result) const;

        void assign_stream(const std::shared_ptr<stream_interface>& stream,
                           std::shared_ptr<stream_profile_interface> target) const;

        std::vector<request_mapping> resolve_requests(stream_profiles requests);
        std::shared_ptr<stream_profile_interface> map_requests(std::shared_ptr<stream_profile_interface> request);

        std::vector<platform::stream_profile> _internal_config;

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::shared_ptr<notifications_processor> _notifications_processor;
        on_before_frame_callback _on_before_frame_callback;
        on_open _on_open;
        std::shared_ptr<metadata_parser_map> _metadata_parsers = nullptr;

        frame_source _source;
        device* _owner;
        std::vector<platform::stream_profile> _uvc_profiles;

    private:
        lazy<stream_profiles> _profiles;
        stream_profiles _active_profiles;
        std::vector<native_pixel_format> _pixel_formats;
        signal<sensor_base, bool> on_before_streaming_changes;
    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual double get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) = 0;
        virtual unsigned long long get_frame_counter(const request_mapping& mode, const platform::frame_object& fo) const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const = 0;
        virtual void reset() = 0;
    };

    class iio_hid_timestamp_reader : public frame_timestamp_reader
    {
        static const int sensors = 2;
        bool started;
        mutable std::vector<int64_t> counter;
        mutable std::recursive_mutex _mtx;
    public:
        iio_hid_timestamp_reader();

        void reset() override;

        rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override;

        bool has_metadata(const request_mapping& mode, const void * metadata, size_t metadata_size) const;

        unsigned long long get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const override;

        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override;
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

        ~hid_sensor() override;

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
        const std::map<rs2_stream, uint32_t> stream_and_fourcc = {{RS2_STREAM_GYRO,  rs_fourcc('G','Y','R','O')},
                                                                  {RS2_STREAM_ACCEL, rs_fourcc('A','C','C','L')},
                                                                  {RS2_STREAM_GPIO,  rs_fourcc('G','P','I','O')}};

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

    class uvc_sensor : public sensor_base
    {
    public:
        explicit uvc_sensor(std::string name, std::shared_ptr<platform::uvc_device> uvc_device,
                            std::unique_ptr<frame_timestamp_reader> timestamp_reader, device* dev);

        virtual ~uvc_sensor() override;

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

        platform::usb_spec get_usb_specification() const { return _device->get_usb_specification(); }
        std::string get_device_path() const { return _device->get_device_location(); }
       
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
                if (auto strong = _owner.lock())
                {
                    try
                    {
                        strong->release_power();
                    }
                    catch (...) {}
                }
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
    };

    processing_blocks get_color_recommended_proccesing_blocks();
    processing_blocks get_depth_recommended_proccesing_blocks();
}
