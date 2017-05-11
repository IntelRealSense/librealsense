// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_HPP
#define LIBREALSENSE_RS2_HPP

#include "rs2.h"
#include "rscore2.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <exception>
#include <ostream>
#include <iostream>

namespace rs2
{
    class error : public std::runtime_error
    {
        std::string function, args;
        rs2_exception_type type;
    public:
        explicit error(rs2_error * err) : runtime_error(rs2_get_error_message(err))
        {
            function = (nullptr != rs2_get_failed_function(err)) ? rs2_get_failed_function(err) : std::string();
            args = (nullptr != rs2_get_failed_args(err)) ? rs2_get_failed_args(err) : std::string();
            type = rs2_get_librealsense_exception_type(err);
            rs2_free_error(err);
        }

        const std::string& get_failed_function() const
        {
            return function;
        }

        const std::string& get_failed_args() const
        {
            return args;
        }

        rs2_exception_type get_type() const { return type; }

        static void handle(rs2_error * e);
    };

#define RS2_ERROR_CLASS(name, base) \
    class name : public base\
    {\
    public:\
        explicit name(rs2_error * e) noexcept : base(e) {}\
    }

    RS2_ERROR_CLASS(recoverable_error, error);
    RS2_ERROR_CLASS(unrecoverable_error, error);
    RS2_ERROR_CLASS(camera_disconnected_error, unrecoverable_error);
    RS2_ERROR_CLASS(backend_error, unrecoverable_error);
    RS2_ERROR_CLASS(device_in_recovery_mode_error, unrecoverable_error);
    RS2_ERROR_CLASS(invalid_value_error, recoverable_error);
    RS2_ERROR_CLASS(wrong_api_call_sequence_error, recoverable_error);
    RS2_ERROR_CLASS(not_implemented_error, recoverable_error);
#undef RS2_ERROR_CLASS

    inline void error::handle(rs2_error * e)
    {
        if (e)
        {
            auto h = rs2_get_librealsense_exception_type(e);
            switch (h) {
            case RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED:
                throw camera_disconnected_error(e);
            case RS2_EXCEPTION_TYPE_BACKEND:
                throw backend_error(e);
            case RS2_EXCEPTION_TYPE_INVALID_VALUE:
                throw invalid_value_error(e);
            case RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE:
                throw wrong_api_call_sequence_error(e);
            case RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED:
                throw not_implemented_error(e);
            case RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE:
                throw device_in_recovery_mode_error(e);
            default:
                throw error(e);
                break;
            }
        }
    }

    class context;
    class device;
    class device_list;
    class syncer;

    struct stream_profile
    {
        rs2_stream stream;
        int width;
        int height;
        int fps;
        rs2_format format;

        bool match(const stream_profile& other) const
        {
            if (stream != RS2_STREAM_ANY && other.stream != RS2_STREAM_ANY && (stream != other.stream))
                return false;
            if (format != RS2_FORMAT_ANY && other.format != RS2_FORMAT_ANY && (format != other.format))
                return false;
            if (fps != 0 && other.fps != 0 && (fps != other.fps))
                return false;
            if (width != 0 && other.width != 0 && (width != other.width))
                return false;
            if (height != 0 && other.height != 0 && (height != other.height))
                return false;
            return true;
        }

        bool contradicts(const std::vector<stream_profile>& requests) const
        {
            for (auto request : requests)
            {
                if (fps != 0 && request.fps != 0 && (fps != request.fps))
                    return true;
                if (width != 0 && request.width != 0 && (width != request.width))
                    return true;
                if (height != 0 && request.height != 0 && (height != request.height))
                    return true;
            }
            return false;
        }

        bool has_wildcards() const
        {
            return (fps == 0 || width == 0 || height == 0 || stream == rs2_stream::RS2_STREAM_ANY || format == RS2_FORMAT_ANY);
        }
    };

    inline bool operator==(const stream_profile& a, const stream_profile& b)
    {
        return (a.width == b.width) && (a.height == b.height) && (a.fps == b.fps) && (a.format == b.format) && (a.stream == b.stream);
    }

    class notification
    {
    public:
        notification(rs2_notification* notification)
        {
            rs2_error * e = nullptr;
            _description = rs2_get_notification_description(notification, &e);
            error::handle(e);
            _timestamp = rs2_get_notification_timestamp(notification, &e);
            error::handle(e);
            _severity = rs2_get_notification_severity(notification, &e);
            error::handle(e);
            _category = rs2_get_notification_category(notification, &e);
            error::handle(e);
        }

        notification()
            : _description(""),
              _timestamp(-1),
              _severity(RS2_LOG_SEVERITY_COUNT),
              _category(RS2_NOTIFICATION_CATEGORY_COUNT)
        {}

        /**
        * retrieve the notification category
        * \return            the notification category
        */
        rs2_notification_category get_category() const
        {
            return _category;
        }
        /**
        * retrieve the notification description
        * \return            the notification description
        */
        std::string get_description() const
        {
            return _description;
        }

        /**
        * retrieve the notification arrival timestamp
        * \return            the arrival timestamp
        */
        double get_timestamp() const
        {
            return _timestamp;
        }

        /**
        * retrieve the notification severity
        * \return            the severity
        */
        rs2_log_severity get_severity() const
        {
            return _severity;
        }

    private:
        std::string _description;
        double _timestamp;
        rs2_log_severity _severity;
        rs2_notification_category _category;
    };

    class frame
    {
    public:
        frame() : frame_ref(nullptr) {}
        frame(rs2_frame * frame_ref) : frame_ref(frame_ref) {}
        frame(frame&& other) : frame_ref(other.frame_ref) { other.frame_ref = nullptr; }
        frame& operator=(frame other)
        {
            swap(other);
            return *this;
        }
        void swap(frame& other)
        {
            std::swap(frame_ref, other.frame_ref);
        }

        /**
        * relases the frame handle
        */
        ~frame()
        {
            if (frame_ref)
            {
                rs2_release_frame(frame_ref);
            }
        }

        operator bool() const { return frame_ref != nullptr; }

        /**
        * retrieve the time at which the frame was captured
        * \return            the timestamp of the frame, in milliseconds since the device was started
        */
        double get_timestamp() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_timestamp(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /** retrieve the timestamp domain
        * \return            timestamp domain (clock name) for timestamp values
        */
        rs2_timestamp_domain get_frame_timestamp_domain() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_timestamp_domain(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /** retrieve the current value of a single frame_metadata
        * \param[in] frame_metadata  the frame_metadata whose value should be retrieved
        * \return            the value of the frame_metadata
        */
        double get_frame_metadata(rs2_frame_metadata frame_metadata) const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r;
        }

        /** determine if the device allows a specific metadata to be queried
        * \param[in] frame_metadata  the frame_metadata to check for support
        * \return            true if the frame_metadata can be queried
        */
        bool supports_frame_metadata(rs2_frame_metadata frame_metadata) const
        {
            rs2_error * e = nullptr;
            auto r = rs2_supports_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r != 0;
        }

        /**
        * retrieve frame number (from frame handle)
        * \return               the frame nubmer of the frame, in milliseconds since the device was started
        */
        unsigned long long get_frame_number() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_number(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve data from frame handle
        * \return               the pointer to the start of the frame data
        */
        const void * get_data() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_data(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image width in pixels
        * \return        frame width in pixels
        */
        int get_width() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_width(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image height in pixels
        * \return        frame height in pixels
        */
        int get_height() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_height(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width)
        * \return            stride in bytes
        */
        int get_stride_in_bytes() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_stride_in_bytes(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve bits per pixel
        * \return            number of bits per one pixel
        */
        int get_bits_per_pixel() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_bits_per_pixel(frame_ref, &e);
            error::handle(e);
            return r;
        }

        int get_bytes_per_pixel() const { return get_bits_per_pixel() / 8; }

        /**
        * retrieve pixel format of the frame
        * \return               pixel format as described in rs2_format enum
        */
        rs2_format get_format() const
        {
            rs2_error * e = nullptr;
            auto r = rs2_get_frame_format(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve the origin stream type that produced the frame
        * \return               stream type of the frame
        */
        rs2_stream get_stream_type() const
        {
            rs2_error * e = nullptr;
            auto s = rs2_get_frame_stream_type(frame_ref, &e);
            error::handle(e);
            return s;
        }

        /**
        * create additional reference to a frame without duplicating frame data
        * \param[out] result     new frame reference, release by destructor
        * \return                true if cloning was successful
        */
        bool try_clone_ref(frame* result) const
        {
            rs2_error * e = nullptr;
            auto s = rs2_clone_frame_ref(frame_ref, &e);
            error::handle(e);
            if (!s) return false;
            *result = frame(s);
            return true;
        }

    private:
        friend class frame_queue;
        friend class syncer;

        rs2_frame* frame_ref;
        frame(const frame&) = delete;
    };



    template<class T>
    class frame_callback : public rs2_frame_callback
    {
        T on_frame_function;
    public:
        explicit frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame * fref) override
        {
            on_frame_function(frame{ fref });
        }

        void release() override { delete this; }
    };

    template<class T>
    class notifications_callback : public rs2_notifications_callback
    {
        T on_notification_function;
    public:
        explicit notifications_callback(T on_notification) : on_notification_function(on_notification) {}

        void on_notification(rs2_notification* _notification) override
        {
            on_notification_function(notification{ _notification });
        }

        void release() override { delete this; }
    };
    struct option_range
    {
        float min;
        float max;
        float def;
        float step;
    };

    struct region_of_interest
    {
        int min_x;
        int min_y;
        int max_x;
        int max_y;
    };

    class advanced
    {
    public:
        explicit advanced(std::shared_ptr<rs2_device> dev)
            : _dev(dev) {}

        std::vector<uint8_t> send_and_receive_raw_data(const std::vector<uint8_t>& input) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<rs2_raw_data_buffer> list(
                rs2_send_and_receive_raw_data(_dev.get(), (void*)input.data(), (uint32_t)input.size(), &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

    private:
        std::shared_ptr<rs2_device> _dev;
    };

    typedef std::vector<frame> frameset;

    class syncer
    {
    public:
        /**
        * Wait until coherent set of frames becomes available
        * \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
        * \return Set of coherent frames
        */
        frameset wait_for_frames(unsigned int timeout_ms = 5000) const
        {
            frameset results;
            std::vector<rs2_frame*> frame_refs(RS2_STREAM_COUNT, nullptr);
            rs2_error* e = nullptr;
            rs2_wait_for_frames(_syncer.get(), timeout_ms, frame_refs.data(), &e);
            error::handle(e);
            for (rs2_frame* ref : frame_refs)
            {
                if (ref) results.emplace_back(ref);
            }
            return results;
        }

        /**
        * Check if a coherent set of frames is available
        * \param[out] result      New coherent frame-set
        * \return true if new frame-set was stored to result
        */
        bool poll_for_frames(frameset* result) const
        {
            std::vector<rs2_frame*> frame_refs(RS2_STREAM_COUNT, nullptr);
            rs2_error* e = nullptr;
            auto res = rs2_poll_for_frames(_syncer.get(), frame_refs.data(), &e);
            error::handle(e);

            if (res)
            {
                result->clear();
                for (rs2_frame* ref : frame_refs)
                {
                    if (ref) result->emplace_back(ref);
                }
                return true;
            }
            return false;
        }

        /**
        * Syncronize new frame
        * \param[in] f - frame handle to enqueue (this operation passed ownership to the syncer)
        */
        void enqueue(frame f) const
        {
            rs2_sync_frame(f.frame_ref, _syncer.get()); // noexcept
            f.frame_ref = nullptr; // frame has been essentially moved from
        }

        void operator()(frame f) const
        {
            enqueue(std::move(f));
        }
    private:
        friend class device;
        std::shared_ptr<rs2_syncer> _syncer;

        syncer(std::shared_ptr<rs2_syncer> s) : _syncer(s) {}
    };

    class device
    {
    public:
        /**
        * open subdevice for exclusive access, by committing to a configuration
        * \param[in] profile    configuration commited by the device
        */
        void open(const stream_profile& profile) const
        {
            rs2_error* e = nullptr;
            rs2_open(_dev.get(),
                profile.stream,
                profile.width,
                profile.height,
                profile.fps,
                profile.format,
                &e);
            error::handle(e);
        }

        /**
        * open subdevice for exclusive access, by committing to composite configuration, specifying one or more stream profiles
        * this method should be used for interdependent  streams, such as depth and infrared, that have to be configured together
        * \param[in] profiles   vector of configurations to be commited by the device
        */
        void open(const std::vector<stream_profile>& profiles) const
        {
            rs2_error* e = nullptr;

            std::vector<rs2_format> formats;
            std::vector<int> widths;
            std::vector<int> heights;
            std::vector<int> fpss;
            std::vector<rs2_stream> streams;
            for (auto& p : profiles)
            {
                formats.push_back(p.format);
                widths.push_back(p.width);
                heights.push_back(p.height);
                fpss.push_back(p.fps);
                streams.push_back(p.stream);
            }

            rs2_open_multiple(_dev.get(),
                streams.data(),
                widths.data(),
                heights.data(),
                fpss.data(),
                formats.data(),
                static_cast<int>(profiles.size()),
                &e);
            error::handle(e);
        }

        /**
        * close subdevice for exclusive access
        * this method should be used for releasing device resource
        */
        void close() const
        {
            rs2_error* e = nullptr;
            rs2_close(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * Start passing frames into user provided callback
        * \param[in] callback   Stream callback, can be any callable object accepting rs2::frame
        */
        template<class T>
        void start(T callback) const
        {
            rs2_error * e = nullptr;
            rs2_start_cpp(_dev.get(), new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * Start passing frames of specific stream into user provided callback
        * \param[in] callback   Stream callback, can be any callable object accepting rs2::frame
        */
        template<class T>
        void start(rs2_stream stream, T callback) const
        {
            rs2_error * e = nullptr;
            rs2_start_stream_cpp(_dev.get(), stream, new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * stop streaming
        */
        void stop() const
        {
            rs2_error * e = nullptr;
            rs2_stop(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * stop streaming of specific stream
        */
        void stop(rs2_stream stream) const
        {
            rs2_error * e = nullptr;
            rs2_stop_stream(_dev.get(), stream, &e);
            error::handle(e);
        }

        /**
        * check if particular option is read-only
        * \param[in] option     option id to be checked
        * \return true if option is read-only
        */
        bool is_option_read_only(rs2_option option)
        {
            rs2_error* e = nullptr;
            auto res = rs2_is_option_read_only(_dev.get(), option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * register notifications callback
        * \param[in] callback   notifications callback
        */
        template<class T>
        void set_notifications_callback(T callback) const
        {
            rs2_error * e = nullptr;
            rs2_set_notifications_callback_cpp(_dev.get(), 
                new notifications_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * read option value from the device
        * \param[in] option   option id to be queried
        * \return value of the option
        */
        float get_option(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option(_dev.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * retrieve the available range of values of a supported option
        * \return option  range containing minimum and maximum values, step and default value
        */
        option_range get_option_range(rs2_option option) const
        {
            option_range result;
            rs2_error* e = nullptr;
            rs2_get_option_range(_dev.get(), option,
                &result.min, &result.max, &result.step, &result.def, &e);
            error::handle(e);
            return result;
        }

        /**
        * write new value to device option
        * \param[in] option     option id to be queried
        * \param[in] value      new value for the option
        */
        void set_option(rs2_option option, float value) const
        {
            rs2_error* e = nullptr;
            rs2_set_option(_dev.get(), option, value, &e);
            error::handle(e);
        }

        /**
        * check if particular option is supported by a subdevice
        * \param[in] option     option id to be checked
        * \return true if option is supported
        */
        bool supports(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_supports_option(_dev.get(), option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * sets region of interest for auto-exposure algorithm
        * \param[in] roi     new region of interest in pixels
        */
        void set_region_of_interest(const region_of_interest& roi)
        {
            rs2_error* e = nullptr;
            rs2_set_region_of_interest(_dev.get(), roi.min_x, roi.min_y, roi.max_x, roi.max_y, &e);
            error::handle(e);
        }

        /**
        * gets region of interest for auto-exposure algorithm
        * \return    current region of interest in pixels
        */
        region_of_interest get_region_of_interest() const
        {
            region_of_interest roi;
            rs2_error* e = nullptr;
            rs2_get_region_of_interest(_dev.get(), &roi.min_x, &roi.min_y, &roi.max_x, &roi.max_y, &e);
            error::handle(e);
            return roi;
        }

        /**
        * get option description
        * \param[in] option     option id to be checked
        * \return human-readable option description
        */
        const char* get_option_description(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_description(_dev.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option value description (in case specific option value hold special meaning)
        * \param[in] option     option id to be checked
        * \param[in] value      value of the option
        * \return human-readable description of a specific value of an option or null if no special meaning
        */
        const char* get_option_value_description(rs2_option option, float val) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_value_description(_dev.get(), option, val, &e);
            error::handle(e);
            return res;
        }

        /**
        * check if physical subdevice is supported
        * \return   list of stream profiles that given subdevice can provide, should be released by rs2_delete_profiles_list
        */
        std::vector<stream_profile> get_stream_modes() const
        {
            std::vector<stream_profile> results;

            rs2_error* e = nullptr;
            std::shared_ptr<rs2_stream_modes_list> list(
                rs2_get_stream_modes(_dev.get(), &e),
                rs2_delete_modes_list);
            error::handle(e);

            auto size = rs2_get_modes_count(list.get(), &e);
            error::handle(e);

            for (auto i = 0; i < size; i++)
            {
                stream_profile profile;
                rs2_get_stream_mode(list.get(), i,
                    &profile.stream,
                    &profile.width,
                    &profile.height,
                    &profile.fps,
                    &profile.format,
                    &e);
                error::handle(e);
                results.push_back(profile);
            }

            return results;
        }

        /*
        * retrieve stream intrinsics
        * \param[in] profile the stream profile to calculate the intrinsics for
        * \return intrinsics object
        */
        rs2_intrinsics get_intrinsics(stream_profile profile) const
        {
            rs2_error* e = nullptr;
            rs2_intrinsics intrinsics;
            rs2_get_stream_intrinsics(_dev.get(),
                profile.stream,
                profile.width,
                profile.height,
                profile.fps,
                profile.format, &intrinsics, &e);
            error::handle(e);
            return intrinsics;
        }

        /**
        * check if specific camera info is supported
        * \param[in] info    the parameter to check for support
        * \return                true if the parameter both exist and well-defined for the specific device
        */
        bool supports(rs2_camera_info info) const
        {
            rs2_error* e = nullptr;
            auto is_supported = rs2_supports_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return is_supported > 0;
        }

        /**
        * retrieve camera specific information, like versions of various internal components
        * \param[in] info     camera info type to retrieve
        * \return             the requested camera info string, in a format specific to the device model
        */
        const char* get_camera_info(rs2_camera_info info) const
        {
            rs2_error* e = nullptr;
            auto result = rs2_get_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return result;
        }

        /**
        * returns the list of adjacent devices, sharing the same physical parent composite device
        * \return            the list of adjacent devices
        */
        std::vector<device> get_adjacent_devices() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device_list> list(
                rs2_query_adjacent_devices(_dev.get(), &e),
                rs2_delete_device_list);
            error::handle(e);

            auto size = rs2_get_device_count(list.get(), &e);
            error::handle(e);

            std::vector<device> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs2_device> dev(
                    rs2_create_device(list.get(), i, &e),
                    rs2_delete_device);
                error::handle(e);

                device rs2_dev(dev);
                results.push_back(rs2_dev);
            }

            return results;
        }

        /**
         * Create frames syncronization primitive suitable for this device
         */
        syncer create_syncer() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_syncer> s(
                rs2_create_syncer(_dev.get(), &e),
                rs2_delete_syncer);
            error::handle(e);
            return syncer(s);
        }

        /**
        * returns the extrinsics from stream on this device to stream on another device
        * \return extrinsics
        */
        rs2_extrinsics get_extrinsics_to(rs2_stream from_stream, const device& to_device, rs2_stream to_stream) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics extrin;
            rs2_get_extrinsics(_dev.get(), from_stream, to_device._dev.get(), to_stream, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        /**
         * returns scale and bias of a motion stream
         * \param stream    Motion stream type (Gyro / Accel / ...)
         */
        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream)
        {
            rs2_error* e = nullptr;
            rs2_motion_device_intrinsic intrin;
            rs2_get_motion_intrinsics(_dev.get(), stream, &intrin, &e);
            error::handle(e);
            return intrin;
        }

        /**
        * send hardware reset request to the device
        */
        void hardware_reset()
        {
            rs2_error* e = nullptr;

            rs2_hardware_reset(_dev.get(), &e);
            error::handle(e);
        }

        /** Retrieves mapping between the units of the depth image and meters
         * This function is deprecated! Please use RS2_OPTION_DEPTH_UNITS instead
         * \return depth in meters corresponding to a depth value of 1
         */
        #ifdef __GNUC__
        __attribute__((deprecated))
        #elif defined(_MSC_VER)
        __declspec(deprecated)
        #endif
        float get_depth_scale() const
        {
            if (supports(RS2_OPTION_DEPTH_UNITS))
                return get_option(RS2_OPTION_DEPTH_UNITS);
            return 1.f;
        }

        advanced& debug() { return _debug; }

        device() : _dev(nullptr), _debug(nullptr) {}

    private:
        friend context;
        friend device_list;

        std::shared_ptr<rs2_device> _dev;
        explicit device(std::shared_ptr<rs2_device> dev)
            : _dev(dev), _debug(dev)
        {
        }

        
        std::shared_ptr<rs2_context> _context;
        advanced _debug;
    };



    inline bool operator==(const device& a, const device& b)
    {
        return (std::string(a.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)) == b.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)) &&
               (std::string(a.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)) == b.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)) &&
               (std::string(a.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER)) == b.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
    }

    inline bool operator!=(const device& a, const device& b)
    {
        return !(a == b);
    }

    class device_list
    {
    public:
        explicit device_list(std::shared_ptr<rs2_device_list> list)
            : _list(move(list)) {}

        device_list()
            : _list(nullptr) {}

        operator std::vector<device>() const
        {
            std::vector<device> res;
            for (auto&& dev : *this) res.push_back(dev);
            return res;
        }

        device operator[](uint32_t index) const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_device(_list.get(), index, &e),
                rs2_delete_device);
            error::handle(e);

            return device(dev);
        }

        uint32_t size() const
        {
            rs2_error* e = nullptr;
            auto size = rs2_get_device_count(_list.get(), &e);
            error::handle(e);
            return size;
        }

        device front() const { return std::move((*this)[0]); }
        device back() const
        {
            return std::move((*this)[size() - 1]);
        }

        class device_list_iterator
        {
            device_list_iterator(
                const device_list& device_list,
                uint32_t uint32_t)
                : _list(device_list),
                  _index(uint32_t)
            {
            }

        public:
            device operator*() const
            {
                return _list[_index];
            }
            bool operator!=(const device_list_iterator& other) const
            {
                return other._index != _index || &other._list != &_list;
            }
            bool operator==(const device_list_iterator& other) const
            {
                return !(*this != other);
            }
            device_list_iterator& operator++()
            {
                _index++;
                return *this;
            }
        private:
            friend device_list;
            const device_list& _list;
            uint32_t _index;
        };

        device_list_iterator begin() const
        {
            return device_list_iterator(*this, 0);
        }
        device_list_iterator end() const
        {
            return device_list_iterator(*this, size());
        }
    private:
        std::shared_ptr<rs2_device_list> _list;
    };

    /**
    * default librealsense context class
    * includes realsense API version as provided by RS2_API_VERSION macro
    */
    class context
    {
    public:
        context()
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_context(RS2_API_VERSION, &e),
                rs2_delete_context);
            error::handle(e);
        }

        /**
        * create a static snapshot of all connected devices at the time of the call
        * \return            the list of devices connected devices at the time of the call
        */
        device_list query_devices() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device_list> list(
                rs2_query_devices(_context.get(), &e),
                rs2_delete_device_list);
            error::handle(e);

            return device_list(list);
        }

        rs2_extrinsics get_extrinsics(const device& from_device, const device& to_device) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics extrin;
            rs2_get_extrinsics(from_device._dev.get(), RS2_STREAM_ANY, to_device._dev.get(), RS2_STREAM_ANY, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        rs2_extrinsics get_extrinsics(const device& from_device, rs2_stream from_stream,
                                      const device& to_device, rs2_stream to_stream) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics extrin;
            rs2_get_extrinsics(from_device._dev.get(), from_stream, to_device._dev.get(), to_stream, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        /**
        * \return            the time at specific time point, in live and redord contextes it will return the system time and in playback contextes it will return the recorded time
        */
        double get_time()
        {
            rs2_error* e = nullptr;
            auto time = rs2_get_context_time(_context.get(), &e);

            error::handle(e);

            return time;
        }


    protected:
        std::shared_ptr<rs2_context> _context;
    };

    class recording_context : public context
    {
    public:
        /**
        * create librealsense context that will try to record all operations over librealsense into a file
        * \param[in] filename string representing the name of the file to record
        */
        recording_context(const std::string& filename,
                          const std::string& section = "",
                          rs2_recording_mode mode = RS2_RECORDING_MODE_BEST_QUALITY)
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_recording_context(RS2_API_VERSION, filename.c_str(), section.c_str(), mode, &e),
                rs2_delete_context);
            error::handle(e);
        }

        recording_context() = delete;
    };

    class mock_context : public context
    {
    public:
        /**
        * create librealsense context that given a file will respond to calls exactly as the recording did
        * if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
        * \param[in] filename string of the name of the file
        */
        mock_context(const std::string& filename,
                     const std::string& section = "")
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_mock_context(RS2_API_VERSION, filename.c_str(), section.c_str(), &e),
                rs2_delete_context);
            error::handle(e);
        }

        mock_context() = delete;
    };

    class frame_queue
    {
    public:
        /**
        * create frame queue. frame queues are the simplest x-platform synchronization primitive provided by librealsense
        * to help developers who are not using async APIs
        * param[in] capacity size of the frame queue
        */
        explicit frame_queue(unsigned int capacity)
        {
            rs2_error* e = nullptr;
            _queue = std::shared_ptr<rs2_frame_queue>(
                rs2_create_frame_queue(capacity, &e),
                rs2_delete_frame_queue);
            error::handle(e);
        }

        frame_queue() : frame_queue(1) {}

        /**
        * release all frames inside the queue
        */
        void flush() const
        {
            rs2_error* e = nullptr;
            rs2_flush_queue(_queue.get(), &e);
            error::handle(e);
        }

        /**
        * enqueue new frame into a queue
        * \param[in] f - frame handle to enqueue (this operation passed ownership to the queue)
        */
        void enqueue(frame f) const
        {
            rs2_enqueue_frame(f.frame_ref, _queue.get()); // noexcept
            f.frame_ref = nullptr; // frame has been essentially moved from
        }

        /**
        * wait until new frame becomes available in the queue and dequeue it
        * \return frame handle to be released using rs2_release_frame
        */
        frame wait_for_frame() const
        {
            rs2_error* e = nullptr;
            auto frame_ref = rs2_wait_for_frame(_queue.get(), &e);
            error::handle(e);
            return{ frame_ref };
        }

        /**
        * poll if a new frame is available and dequeue if it is
        * \param[out] f - frame handle
        * \return true if new frame was stored to f
        */
        bool poll_for_frame(frame* f) const
        {
            rs2_error* e = nullptr;
            rs2_frame* frame_ref = nullptr;
            auto res = rs2_poll_for_frame(_queue.get(), &frame_ref, &e);
            error::handle(e);
            if (res) *f = { frame_ref };
            return res > 0;
        }

        void operator()(frame f) const
        {
            enqueue(std::move(f));
        }

    private:
        std::shared_ptr<rs2_frame_queue> _queue;
    };

    inline void log_to_console(rs2_log_severity min_severity)
    {
        rs2_error * e = nullptr;
        rs2_log_to_console(min_severity, &e);
        error::handle(e);
    }

    inline void log_to_file(rs2_log_severity min_severity, const char * file_path = nullptr)
    {
        rs2_error * e = nullptr;
        rs2_log_to_file(min_severity, file_path, &e);
        error::handle(e);
    }
}

inline std::ostream & operator << (std::ostream & o, rs2_stream stream) { return o << rs2_stream_to_string(stream); }
inline std::ostream & operator << (std::ostream & o, rs2_format format) { return o << rs2_format_to_string(format); }
inline std::ostream & operator << (std::ostream & o, rs2_distortion distortion) { return o << rs2_distortion_to_string(distortion); }
inline std::ostream & operator << (std::ostream & o, rs2_option option) { return o << rs2_option_to_string(option); }
inline std::ostream & operator << (std::ostream & o, rs2_log_severity severity) { return o << rs2_log_severity_to_string(severity); }

#endif // LIBREALSENSE_RS2_HPP
