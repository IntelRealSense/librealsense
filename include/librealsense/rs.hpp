// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rs.h"
#include "rscore.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <exception>
#include <ostream>
#include <iostream>

namespace rs
{

    class error : public std::runtime_error
    {
        std::string function, args;
    public:
        explicit error(rs_error * err) : std::runtime_error(rs_get_error_message(err))
        {
            function = (nullptr != rs_get_failed_function(err)) ? rs_get_failed_function(err) : std::string();
            args = (nullptr != rs_get_failed_args(err)) ? rs_get_failed_args(err) : std::string();
            rs_free_error(err);
        }
        const std::string & get_failed_function() const { return function; }
        const std::string & get_failed_args() const { return args; }
        static void handle(rs_error * e) { if (e) throw error(e); }
    };

    class context;
    class device;
    class subdevice;

    struct stream_profile
    {
        rs_stream stream;
        int width;
        int height;
        int fps;
        rs_format format;
    };

    class frame
    {
        rs_frame * frame_ref;
        frame(const frame &) = delete;
    public:
        friend class frame_queue;

        frame() : frame_ref(nullptr) {}
        frame(rs_frame * frame_ref) : frame_ref(frame_ref) {}
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
                rs_release_frame(frame_ref);
            }
        }

        operator bool() const { return frame_ref != nullptr; }

        /**
        * retrieve the time at which the frame was captured
        * \return            the timestamp of the frame, in milliseconds since the device was started
        */
        double get_timestamp() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /* retrieve the timestamp domain
        * \return            timestamp domain (clock name) for timestamp values
        */
        rs_timestamp_domain get_frame_timestamp_domain() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp_domain(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /* retrieve the current value of a single frame_metadata
        * \param[in] frame_metadata  the frame_metadata whose value should be retrieved
        * \return            the value of the frame_metadata
        */
        double get_frame_metadata(rs_frame_metadata frame_metadata) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r;
        }

        /* determine if the device allows a specific metadata to be queried
        * \param[in] frame_metadata  the frame_metadata to check for support
        * \return            true if the frame_metadata can be queried
        */
        bool supports_frame_metadata(rs_frame_metadata frame_metadata) const
        {
            rs_error * e = nullptr;
            auto r = rs_supports_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r != 0;
        }

        /**
        * retrive frame number (from frame handle)
        * \return               the frame nubmer of the frame, in milliseconds since the device was started
        */
        unsigned long long get_frame_number() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_number(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrive data from frame handle
        * \return               the pointer to the start of the frame data
        */
        const void * get_data() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_data(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image width in pixels
        * \return        frame width in pixels
        */
        int get_width() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_width(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image height in pixels
        * \return        frame height in pixels
        */
        int get_height() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_height(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrive frame stride, meaning the actual line width in memory in bytes (not the logical image width)
        * \return            stride in bytes
        */
        int get_stride_in_bytes() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_stride_in_bytes(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve bits per pixel
        * \return            number of bits per one pixel
        */
        int get_bits_per_pixel() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_bits_per_pixel(frame_ref, &e);
            error::handle(e);
            return r;
        }

        int get_bytes_per_pixel() const { return get_bits_per_pixel() / 8; }

        /**
        * retrive pixel format of the frame
        * \return               pixel format as described in rs_format enum
        */
        rs_format get_format() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_format(frame_ref, &e);
            error::handle(e);
            return r;;
        }

        /**
        * retrive the origin stream type that produced the frame
        * \return               stream type of the frame
        */
        rs_stream get_stream_type() const
        {
            rs_error * e = nullptr;
            auto s = rs_get_frame_stream_type(frame_ref, &e);
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
            rs_error * e = nullptr;
            auto s = rs_clone_frame_ref(frame_ref, &e);
            error::handle(e);
            if (!s) return false;
            *result = frame(s);
            return true;
        }
    };

    template<class T>
    class frame_callback : public rs_frame_callback
    {
        T on_frame_function;
    public:
        explicit frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs_frame * fref) override
        {
            on_frame_function({ fref });
        }

        void release() override { delete this; }
    };

    class streaming_lock
    {
    public:

        /**
        * start streaming from specified configured device
        * \param[in] callback callback object created from c++ application. ownership over the callback object is moved into the relevant streaming lock
        */
        template<class T>
        void start(T callback) const
        {
            rs_error * e = nullptr;
            rs_start_cpp(_lock.get(), new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * stops streaming from specified configured device
        */
        void stop() const
        {
            rs_error * e = nullptr;
            rs_stop(_lock.get(), &e);
            error::handle(e);
        }
        streaming_lock() : _lock(nullptr) {}
    private:
        friend subdevice;
        explicit streaming_lock(std::shared_ptr<rs_streaming_lock> lock)
            : _lock(std::move(lock)) {}

        std::shared_ptr<rs_streaming_lock> _lock;
    };

    struct option_range
    {
        float min;
        float max;
        float def;
        float step;
    };

    class subdevice
    {
    public:
        subdevice() : _dev(nullptr), _index(RS_SUBDEVICE_COLOR) {}
        operator rs_subdevice() const { return _index; }

        /**
        * open subdevice for exclusive access, by commiting to a configuration
        * \param[in] profile    configuration commited by the device
        * \return               exclusive lock to be used for streaming
        */
        streaming_lock open(const stream_profile& profile) const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_streaming_lock> lock(
                rs_open(_dev, _index,
                    profile.stream,
                    profile.width,
                    profile.height,
                    profile.fps,
                    profile.format,
                    &e),
                rs_close);
            error::handle(e);

            return streaming_lock(lock);
        }

        /**
        * open subdevice for exclusive access, by commiting to composite configuration, specifying one or more stream profiles
        * this method should be used for interdendent streams, such as depth and infrared, that have to be configured together
        * \param[in] vector of configurations to be commited by the device
        * \return exclusive lock to be used for streaming
        */
        streaming_lock open(const std::vector<stream_profile>& profiles) const
        {
            rs_error* e = nullptr;

            std::vector<rs_format> formats;
            std::vector<int> widths;
            std::vector<int> heights;
            std::vector<int> fpss;
            std::vector<rs_stream> streams;
            for (auto& p : profiles)
            {
                formats.push_back(p.format);
                widths.push_back(p.width);
                heights.push_back(p.height);
                fpss.push_back(p.fps);
                streams.push_back(p.stream);
            }

            std::shared_ptr<rs_streaming_lock> lock(
                rs_open_many(_dev, _index,
                    streams.data(),
                    widths.data(),
                    heights.data(),
                    fpss.data(),
                    formats.data(),
                    static_cast<int>(profiles.size()),
                    &e),
                rs_close);
            error::handle(e);

            return streaming_lock(lock);
        }

        /**
        * read option value from the device
        * \param[in] option   option id to be queried
        * \return value of the option
        */
        float get_option(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_subdevice_option(_dev, _index, option, &e);
            error::handle(e);
            return res;
        }

        /**
        * retrieve the available range of values of a supported option
        * \return option range containing minimum and maximum values, step and default value
        */
        option_range get_option_range(rs_option option) const
        {
            option_range result;
            rs_error* e = nullptr;
            rs_get_subdevice_option_range(_dev, _index, option,
                &result.min, &result.max, &result.step, &result.def, &e);
            error::handle(e);
            return result;
        }

        /**
        * write new value to device option
        * \param[in] option     option id to be queried
        * \param[in] value      new value for the option
        */
        void set_option(rs_option option, float value) const
        {
            rs_error* e = nullptr;
            rs_set_subdevice_option(_dev, _index, option, value, &e);
            error::handle(e);
        }

        /**
        * check if particular option is supported by a subdevice
        * \param[in] option     option id to be checked
        * \return true if option is supported
        */
        bool supports(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_supports_subdevice_option(_dev, _index, option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * get option description
        * \param[in] option     option id to be checked
        * \return human-readable option description
        */
        const char* get_option_description(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_subdevice_option_description(_dev, _index, option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option value description (in case specific option value hold special meaning)
        * \param[in] option     option id to be checked
        * \param[in] value      value of the option
        * \return human-readable description of a specific value of an option or null if no special meaning
        */
        const char* get_option_value_description(rs_option option, float val) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_subdevice_option_value_description(_dev, _index, option, val, &e);
            error::handle(e);
            return res;
        }

        /**
        * check if physical subdevice is supported
        * \return   list of stream profiles that given subdevice can provide, should be released by rs_delete_profiles_list
        */
        std::vector<stream_profile> get_stream_modes() const
        {
            std::vector<stream_profile> results;

            rs_error* e = nullptr;
            std::shared_ptr<rs_stream_modes_list> list(
                rs_get_stream_modes(_dev, _index, &e),
                rs_delete_modes_list);
            error::handle(e);

            auto size = rs_get_modes_count(list.get(), &e);
            error::handle(e);

            for (auto i = 0; i < size; i++)
            {
                stream_profile profile;
                rs_get_stream_mode(list.get(), i,
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
         * retrive stream intrinsics
         * \param[in] profile the stream profile to calculate the intrinsics for
         * \return intrinsics object
         */
        rs_intrinsics get_intrinsics(stream_profile profile) const
        {
            rs_error* e = nullptr;
            rs_intrinsics intrinsics;
            rs_get_stream_intrinsics(_dev, _index,
                profile.stream,
                profile.width,
                profile.height,
                profile.fps,
                profile.format, &intrinsics, &e);
            error::handle(e);
            return intrinsics;
        }

    private:
        friend device;
        explicit subdevice(rs_device* dev, rs_subdevice index)
            : _dev(dev), _index(index) {}

        rs_device* _dev;
        rs_subdevice _index;
    };

    class advanced
    {
    public:
        advanced(std::shared_ptr<rs_device> dev)
            : _dev(dev)
        {}
        std::vector<uint8_t> send_and_receive_raw_data(const std::vector<uint8_t>& input) const
        {
            std::vector<uint8_t> results;

            rs_error* e = nullptr;
            std::shared_ptr<rs_raw_data_buffer> list(
                rs_send_and_receive_raw_data(_dev.get(), (void*)input.data(), input.size(), &e),
                rs_delete_raw_data);
            error::handle(e);

            auto size = rs_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

    private:
        std::shared_ptr<rs_device> _dev;
    };

    class device
    {
    public:
        subdevice& get_subdevice(rs_subdevice sub)
        {
            if (sub < _subdevices.size() && _subdevices[sub].get())
                return *_subdevices[sub];
            throw std::runtime_error("Requested subdevice is not supported!");
        }

        bool supports(rs_subdevice sub) const
        {
            return sub < _subdevices.size() && _subdevices[sub].get();
        }

        subdevice& color() { return get_subdevice(RS_SUBDEVICE_COLOR); }
        subdevice& depth() { return get_subdevice(RS_SUBDEVICE_DEPTH); }
        subdevice& fisheye() { return get_subdevice(RS_SUBDEVICE_FISHEYE); }
        subdevice& motion() { return get_subdevice(RS_SUBDEVICE_MOTION); }

        /**
        * check if specific camera info is supported
        * \param[in] info    the parameter to check for support
        * \return                true if the parameter both exist and well-defined for the specific device
        */
        bool supports(rs_camera_info info) const
        {
            rs_error* e = nullptr;
            auto is_supported = rs_supports_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return is_supported > 0;
        }

        /**
        * retrieve camera specific information, like versions of various internal componnents
        * \param[in] info     camera info type to retrieve
        * \return             the requested camera info string, in a format specific to the device model
        */
        const char* get_camera_info(rs_camera_info info) const
        {
            rs_error* e = nullptr;
            auto result = rs_get_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return result;
        }

        rs_extrinsics get_extrinsics(rs_subdevice from_subdevice, rs_subdevice to_subdevice) const
        {
            rs_error* e = nullptr;
            rs_extrinsics extrin;
            rs_get_device_extrinsics(_dev.get(), from_subdevice, to_subdevice, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        /**
        * retrieve mapping between the units of the depth image and meters
        * \return            depth in meters corresponding to a depth value of 1
        */
        float get_depth_scale() const
        {
            rs_error* e = nullptr;
            auto result = rs_get_device_depth_scale(_dev.get(), &e);
            error::handle(e);
            return result;
        }
        advanced& debug() { return _debug; }
    private:
        friend context;
        explicit device(std::shared_ptr<rs_device> dev) : _dev(dev), _debug(dev)
        {
            _subdevices.resize(RS_SUBDEVICE_COUNT);
            for (auto i = 0; i < RS_SUBDEVICE_COUNT; i++)
            {
                auto s = static_cast<rs_subdevice>(i);
                rs_error* e = nullptr;
                auto is_supported = rs_is_subdevice_supported(_dev.get(), s, &e);
                error::handle(e);
                if (is_supported) _subdevices[s].reset(new subdevice(_dev.get(), s));
                else  _subdevices[s] = nullptr;
            }
        }

        std::shared_ptr<rs_device> _dev;
        advanced _debug;
        std::vector<std::shared_ptr<subdevice>> _subdevices;
    };

    class subdevice_iterator
    {
    public:
        subdevice_iterator(device dev, rs_subdevice idx)
            : _idx(idx), _dev(dev) {}

        subdevice_iterator& operator++()
        {
            do
            {
                _idx = static_cast<rs_subdevice>(static_cast<int>(_idx) + 1);
            } while (_idx != RS_SUBDEVICE_COUNT && !_dev.supports(_idx));
            return *this;
        }

        subdevice& operator*() { return _dev.get_subdevice(_idx); }
        bool operator==(const subdevice_iterator& other) const
        {
            return _idx == other._idx;
        }
        bool operator!=(const subdevice_iterator& other) const
        {
            return _idx != other._idx;
        }
    private:
        rs_subdevice _idx;
        device _dev;
    };

    inline subdevice_iterator begin(device dev) { return{ dev, RS_SUBDEVICE_COLOR }; }
    inline subdevice_iterator end(device dev) { return{ dev, RS_SUBDEVICE_COUNT }; }

    /**
    * default librealsense context class
    * includes realsense API version as provided by RS_API_VERSION macro
    */
    class context
    {
    public:
        context()
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_context(RS_API_VERSION, &e),
                rs_delete_context);
            error::handle(e);
        }

        /**
        * create a static snapshot of all connected devices at the time of the call
        * \return            the list of devices connected devices at the time of the call
        */
        std::vector<device> query_devices() const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device_list> list(
                rs_query_devices(_context.get(), &e),
                rs_delete_device_list);
            error::handle(e);

            auto size = rs_get_device_count(list.get(), &e);
            error::handle(e);

            std::vector<device> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs_device> dev(
                    rs_create_device(list.get(), i, &e),
                    rs_delete_device);
                error::handle(e);

                device rs_dev(dev);
                results.push_back(rs_dev);
            }

            return results;
        }

    protected:
        std::shared_ptr<rs_context> _context;
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
                          rs_recording_mode mode = RS_RECORDING_MODE_BEST_QUALITY)
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_recording_context(RS_API_VERSION, filename.c_str(), section.c_str(), mode, &e),
                rs_delete_context);
            error::handle(e);
        }

        recording_context() = delete;
    };

    class mock_context : public context
    {
    public:
        /**
        * create librealsense context that given a file will respond to calls exactly as the recording did
        * if the user calls a method that was either not called during recording or voilates causality of the recording error will be thrown
        * \param[in] filename string of the name of the file
        */
        mock_context(const std::string& filename, 
                     const std::string& section = "")
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_mock_context(RS_API_VERSION, filename.c_str(), section.c_str(), &e),
                rs_delete_context);
            error::handle(e);
        }

        mock_context() = delete;
    };

    class frame_queue
    {
    public:
        /**
        * create frame queue. frame queues are the simplest x-platform syncronization primitive provided by librealsense
        * to help developers who are not using async APIs
        * param[in] capacity size of the frame queue
        */
        explicit frame_queue(unsigned int capacity)
        {
            rs_error* e = nullptr;
            _queue = std::shared_ptr<rs_frame_queue>(
                rs_create_frame_queue(capacity, &e),
                rs_delete_frame_queue);
            error::handle(e);
        }

        frame_queue() : frame_queue(1) {}

        ~frame_queue()
        {
            flush();
        }

        /**
        * release all frames inside the queue
        */
        void flush() const
        {
            rs_error* e = nullptr;
            rs_flush_queue(_queue.get(), &e);
            error::handle(e);
        }

        /**
        * enqueue new frame into a queue
        * \param[in] f - frame handle to enqueue (this operation passed ownership to the queue)
        */
        void enqueue(frame f) const
        {
            rs_enqueue_frame(f.frame_ref, _queue.get()); // noexcept
            f.frame_ref = nullptr; // frame has been essentially moved from
        }

        /**
        * wait until new frame becomes available in the queue and dequeue it
        * \return frame handle to be released using rs_release_frame
        */
        frame wait_for_frame() const
        {
            rs_error* e = nullptr;
            auto frame_ref = rs_wait_for_frame(_queue.get(), &e);
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
            rs_error* e = nullptr;
            rs_frame* frame_ref = nullptr;
            auto res = rs_poll_for_frame(_queue.get(), &frame_ref, &e);
            error::handle(e);
            if (res) *f = { frame_ref };
            return res > 0;
        }

        void operator()(frame f) const
        {
            enqueue(std::move(f));
        }

    private:
        std::shared_ptr<rs_frame_queue> _queue;
    };

    inline void log_to_console(rs_log_severity min_severity)
    {
        rs_error * e = nullptr;
        rs_log_to_console(min_severity, &e);
        error::handle(e);
    }

    inline void log_to_file(rs_log_severity min_severity, const char * file_path = nullptr)
    {
        rs_error * e = nullptr;
        rs_log_to_file(min_severity, file_path, &e);
        error::handle(e);
    }
}

inline std::ostream & operator << (std::ostream & o, rs_stream stream) { return o << rs_stream_to_string(stream); }
inline std::ostream & operator << (std::ostream & o, rs_format format) { return o << rs_format_to_string(format); }
inline std::ostream & operator << (std::ostream & o, rs_distortion distortion) { return o << rs_distortion_to_string(distortion); }
inline std::ostream & operator << (std::ostream & o, rs_option option) { return o << rs_option_to_string(option); }
inline std::ostream & operator << (std::ostream & o, rs_subdevice evt) { return o << rs_subdevice_to_string(evt); }

#endif // LIBREALSENSE_RS_HPP
