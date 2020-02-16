// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"

#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <limits.h>
#include <atomic>
#include <functional>
#include <core/debug.h>

#include "archive.h"
#include "core/streaming.h"
#include "core/roi.h"
#include "core/options.h"
#include "source.h"
#include "core/extension.h"
#include "proc/processing-blocks-factory.h"
#include "proc/identity-processing-block.h"

namespace librealsense
{
    class device;
    class option;

    typedef std::function<void(std::vector<platform::stream_profile>)> on_open;

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() {}

        virtual double get_frame_timestamp(const std::shared_ptr<frame_interface>& frame) = 0;
        virtual unsigned long long get_frame_counter(const std::shared_ptr<frame_interface>& frame) const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const = 0;
        virtual void reset() = 0;
    };

    class sensor_base : public std::enable_shared_from_this<sensor_base>,
                        public virtual sensor_interface, public options_container, public virtual info_container, public recommended_proccesing_blocks_base
    {
    public:
        explicit sensor_base(std::string name,
                             device* device, 
                             recommended_proccesing_blocks_interface* owner);
        virtual ~sensor_base() override { _source.flush(); }

        void set_source_owner(sensor_base* owner); // will direct the source to the top in the source hierarchy.
        virtual stream_profiles init_stream_profiles() = 0;
        stream_profiles get_stream_profiles(int tag = profile_tag::PROFILE_TAG_ANY) const override;
        stream_profiles get_active_streams() const override;
        notifications_callback_ptr get_notifications_callback() const override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        int register_before_streaming_changes_callback(std::function<void(bool)> callback) override;
        void unregister_before_start_callback(int token) override;
        virtual std::shared_ptr<notifications_processor> get_notifications_processor() const;
        virtual frame_callback_ptr get_frames_callback() const override;
        virtual void set_frames_callback(frame_callback_ptr callback) override;
        bool is_streaming() const override;
        virtual bool is_opened() const;
        virtual void register_metadata(rs2_frame_metadata_value metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser) const;
        void register_on_open(on_open callback)
        {
            _on_open = callback;
        }
        device_interface& get_device() override;

        // Make sensor inherit its owning device info by default
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

        processing_blocks get_recommended_processing_blocks() const override
        {
            return {};
        }

        std::shared_ptr<std::map<uint32_t, rs2_format>>& get_fourcc_to_rs2_format_map();
        std::shared_ptr<std::map<uint32_t, rs2_stream>>& get_fourcc_to_rs2_stream_map();

        rs2_format fourcc_to_rs2_format(uint32_t format) const;
        rs2_stream fourcc_to_rs2_stream(uint32_t fourcc_format) const;

    protected:
        void raise_on_before_streaming_changes(bool streaming);
        void set_active_streams(const stream_profiles& requests);

        void assign_stream(const std::shared_ptr<stream_interface>& stream,
                           std::shared_ptr<stream_profile_interface> target) const;

        std::shared_ptr<frame> generate_frame_from_data(const platform::frame_object& fo,
            frame_timestamp_reader* timestamp_reader,
            const rs2_time_t& last_timestamp,
            const unsigned long long& last_frame_number,
            std::shared_ptr<stream_profile_interface> profile);

        std::vector<platform::stream_profile> _internal_config;

        std::atomic<bool> _is_streaming;
        std::atomic<bool> _is_opened;
        std::shared_ptr<notifications_processor> _notifications_processor;
        on_open _on_open;
        std::shared_ptr<metadata_parser_map> _metadata_parsers = nullptr;

        sensor_base* _source_owner;
        frame_source _source;
        device* _owner;
        std::vector<platform::stream_profile> _uvc_profiles;

        std::shared_ptr<std::map<uint32_t, rs2_format>> _fourcc_to_rs2_format;
        std::shared_ptr<std::map<uint32_t, rs2_stream>> _fourcc_to_rs2_stream;

    private:
        lazy<stream_profiles> _profiles;
        stream_profiles _active_profiles;
        mutable std::mutex _active_profile_mutex;
        signal<sensor_base, bool> on_before_streaming_changes;
    };

    class processing_block;

    class motion_sensor : public recordable<motion_sensor>
    {
    public:
        virtual ~motion_sensor() = default;

        void create_snapshot(std::shared_ptr<motion_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const motion_sensor&)> recording_function) override {};
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION_SENSOR, librealsense::motion_sensor);

    class motion_sensor_snapshot : public virtual motion_sensor, public extension_snapshot
    {
    public:
        motion_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
        }

        void create_snapshot(std::shared_ptr<motion_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<motion_sensor_snapshot>(*this);
        }

        void enable_recording(std::function<void(const motion_sensor&)> recording_function) override
        {
            //empty
        }
    };


    class fisheye_sensor : public recordable<fisheye_sensor>
    {
    public:
        virtual ~fisheye_sensor() = default;

        void create_snapshot(std::shared_ptr<fisheye_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const fisheye_sensor&)> recording_function) override {};
    };

    MAP_EXTENSION(RS2_EXTENSION_FISHEYE_SENSOR, librealsense::fisheye_sensor);

    class fisheye_sensor_snapshot : public virtual fisheye_sensor, public extension_snapshot
    {
    public:
        fisheye_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
        }

        void create_snapshot(std::shared_ptr<fisheye_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<fisheye_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const fisheye_sensor&)> recording_function) override
        {
            //empty
        }
    };

    class synthetic_sensor :
        public sensor_base
    {
    public:
        explicit synthetic_sensor(std::string name,
            std::shared_ptr<sensor_base> sensor,
            device* device,
            const std::map<uint32_t, rs2_format>& fourcc_to_rs2_format_map = std::map<uint32_t, rs2_format>(),
            const std::map<uint32_t, rs2_stream>& fourcc_to_rs2_stream_map = std::map<uint32_t, rs2_stream>());
        ~synthetic_sensor() override;

        virtual void register_option(rs2_option id, std::shared_ptr<option> option);
        void unregister_option(rs2_option id);
        void register_pu(rs2_option id);

        virtual stream_profiles init_stream_profiles() override;

        void open(const stream_profiles& requests) override;
        void close() override;
        void start(frame_callback_ptr callback) override;
        void stop() override;

        void register_processing_block(const std::vector<stream_profile>& from,
            const std::vector<stream_profile>& to,
            std::function<std::shared_ptr<processing_block>(void)> generate_func);
        void register_processing_block(const processing_block_factory& pbf);
        void register_processing_block(const std::vector<processing_block_factory>& pbfs);

        std::shared_ptr<sensor_base> get_raw_sensor() const { return _raw_sensor; };
        frame_callback_ptr get_frames_callback() const override;
        void set_frames_callback(frame_callback_ptr callback) override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        int register_before_streaming_changes_callback(std::function<void(bool)> callback) override;
        void unregister_before_start_callback(int token) override;
        void register_metadata(rs2_frame_metadata_value metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser) const override;
        bool is_streaming() const override;
        bool is_opened() const override;

    protected:
        void add_source_profiles_missing_data();

    private:
        stream_profiles resolve_requests(const stream_profiles& requests);
        std::shared_ptr<stream_profile_interface> filter_frame_by_requests(const frame_interface* f);
        void sort_profiles(stream_profiles * profiles);
        std::pair<std::shared_ptr<processing_block_factory>, stream_profiles> find_requests_best_pb_match(const stream_profiles& sp);
        void add_source_profile_missing_data(std::shared_ptr<stream_profile_interface>& source_profile);
        bool is_duplicated_profile(const std::shared_ptr<stream_profile_interface>& duplicate, const stream_profiles& profiles);
        std::shared_ptr<stream_profile_interface> clone_profile(const std::shared_ptr<stream_profile_interface>& profile);
        void register_processing_block_options(const processing_block& pb);
        void unregister_processing_block_options(const processing_block& pb);

        std::mutex _synthetic_configure_lock;

        frame_callback_ptr _post_process_callback;
        std::shared_ptr<sensor_base> _raw_sensor;
        std::vector<std::shared_ptr<processing_block_factory>> _pb_factories;
        std::unordered_map<processing_block_factory*, stream_profiles> _pbf_supported_profiles;
        std::unordered_map<std::shared_ptr<stream_profile_interface>, std::unordered_set<std::shared_ptr<processing_block>>> _profiles_to_processing_block;
        std::unordered_map<std::shared_ptr<stream_profile_interface>, stream_profiles> _source_to_target_profiles_map;
        std::unordered_map<stream_profile, stream_profiles> _target_to_source_profiles_map;
        std::unordered_map<rs2_format, stream_profiles> _cached_requests;
        std::vector<rs2_option> _cached_processing_blocks_options;
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

        rs2_time_t get_frame_timestamp(const std::shared_ptr<frame_interface>& frame) override;

        bool has_metadata(const std::shared_ptr<frame_interface>& frame) const;

        unsigned long long get_frame_counter(const std::shared_ptr<frame_interface>& frame) const override;

        rs2_timestamp_domain get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const override;
    };

    class hid_sensor : public sensor_base
    {
    public:
        explicit hid_sensor(std::shared_ptr<platform::hid_device> hid_device,
                            std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
                            std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
                            const std::map<rs2_stream, std::map<unsigned, unsigned>>& fps_and_sampling_frequency_per_rs2_stream,
                            const std::vector<std::pair<std::string, stream_profile>>& sensor_name_and_hid_profiles,
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
        std::map<std::string, std::shared_ptr<stream_profile_interface>> _configured_profiles;
        std::vector<bool> _is_configured_stream;
        std::vector<platform::hid_sensor> _hid_sensors;
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
        void start(frame_callback_ptr callback) override;
        void stop() override;
        void register_xu(platform::extension_unit xu);
        void register_pu(rs2_option id);
        void try_register_pu(rs2_option id);

        std::vector<platform::stream_profile> get_configuration() const { return _internal_config; }
        std::shared_ptr<platform::uvc_device> get_uvc_device() { return _device; }
        platform::usb_spec get_usb_specification() const { return _device->get_usb_specification(); }
        std::string get_device_path() const { return _device->get_device_location(); }

        template<class T>
        auto invoke_powered(T action)
            -> decltype(action(*static_cast<platform::uvc_device*>(nullptr)))
        {
            power on(std::dynamic_pointer_cast<uvc_sensor>(shared_from_this()));
            return action(*_device);
        }

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
