// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"
#include <functional>

namespace rsimpl2
{
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

    struct option_range
    {
        float min;
        float max;
        float step;
        float def;
    };

    class option
    {
    public:
        virtual void set(float value) = 0;
        virtual float query() const = 0;
        virtual option_range get_range() const = 0;
        virtual bool is_enabled() const = 0;
        virtual bool is_read_only() const { return false; }

        virtual const char* get_description() const = 0;
        virtual const char* get_value_description(float) const { return nullptr; }

        virtual ~option() = default;
    };

    class stream_profile_interface
    {
    public:
        virtual bool  is_recommended() const = 0;
        virtual std::size_t get_size() const = 0;
        virtual unsigned int get_fps() const = 0;

        virtual ~stream_profile_interface() = default;
    };

    class sensor_interface;

    class frame_interface
    {
    public:
        virtual double get_timestamp() const = 0;
        virtual rs2_timestamp_domain get_timestamp_domain() const = 0;

        virtual unsigned int get_stream_index() const = 0;

        virtual const uint8_t* get_data() const = 0;
        virtual std::size_t get_data_size() const = 0;

        virtual const sensor_interface& get_sensor() const = 0;

        virtual ~frame_interface() = default;
    };

    using on_frame = std::function<void(frame_interface*)>;
    using stream_profiles = std::vector<std::shared_ptr<stream_profile_interface>>;

    class sensor_interface
    {
    public:
        virtual std::vector<stream_profile> get_principal_requests() = 0;
        virtual void open(const std::vector<stream_profile>& requests) = 0;
        virtual void close() = 0;
        virtual option& get_option(rs2_option id) = 0;
        virtual const option& get_option(rs2_option id) const = 0;

        virtual const std::string& get_info(rs2_camera_info info) const = 0;
        virtual bool supports_info(rs2_camera_info info) const = 0;
        virtual bool supports_option(rs2_option id) const = 0;

        virtual region_of_interest_method& get_roi_method() const = 0;
        virtual void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method) = 0;
        virtual void register_notifications_callback(notifications_callback_ptr callback) = 0;

        virtual pose get_pose() const = 0;

        virtual void start(frame_callback_ptr callback) = 0;
        virtual void stop() = 0;

//        virtual rs2_stream   get_stream_type(unsigned int index) const = 0;
//        virtual const char*  get_stream_desciption(unsigned int index) const = 0;
//        virtual unsigned int get_streams_count() const = 0;

//        virtual stream_profiles get_profiles() const = 0;

//        virtual void start(const stream_profile_interface& profile) = 0;

//        virtual void stop() = 0;

//        virtual void set_callback(on_frame callback) = 0;

//        virtual bool is_streaming() const = 0;

        virtual ~sensor_interface() = default;
    };

    class device_interface
    {
    public:
        virtual sensor_interface& get_sensor(size_t i) = 0;
        virtual size_t get_sensors_count() const = 0;

        virtual ~device_interface() = default;
    };
}
