// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "backend.h"
#include "types.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <thread>
#include <utility> // for pair
#include <chrono>
#include <thread>
#include <atomic>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <fts.h>
#include <regex>
#include <list>

namespace librealsense
{
    namespace platform
    {
        struct hid_input_info
        {
            std::string input = "";
            std::string device_path = "";
            int index = -1;
            bool enabled = false;

            uint32_t big_endian = 0;
            uint32_t bits_used = 0;
            uint32_t bytes = 0;
            uint32_t is_signed = 0;
            uint32_t location = 0;
            uint32_t shift = 0;
            uint64_t mask;
            // TODO: parse 'offset' and 'scale'
        };

        // manage an IIO input. or what is called a scan.
        class hid_input
        {
        public:
            hid_input(const std::string& iio_device_path, const std::string& input_name);
            ~hid_input();

            // enable scan input. doing so cause the input to be part of the data provided in the polling.
            void enable(bool is_enable);

            const hid_input_info& get_hid_input_info() const { return info; }

        private:
            // initialize the input by reading the input parameters.
            void init();

            hid_input_info info;
        };

        class hid_custom_sensor {
        public:
            hid_custom_sensor(const std::string& device_path, const std::string& sensor_name);

            ~hid_custom_sensor();

            std::vector<uint8_t> get_report_data(const std::string& report_name, custom_sensor_report_field report_field);

            const std::string& get_sensor_name() const { return _custom_sensor_name; }

            // start capturing and polling.
            void start_capture(hid_callback sensor_callback);

            void stop_capture();
        private:
            std::vector<uint8_t> read_report(const std::string& name_report_path);

            void init();

            void enable(bool state);

            void signal_stop();

            int _stop_pipe_fd[2]; // write to _stop_pipe_fd[1] and read from _stop_pipe_fd[0]
            int _fd;
            std::map<std::string, std::string> _reports;
            std::string _custom_device_path;
            std::string _custom_sensor_name;
            std::string _custom_device_name;
            hid_callback _callback;
            std::atomic<bool> _is_capturing;
            std::unique_ptr<std::thread> _hid_thread;
        };

        // declare device sensor with all of its inputs.
        class iio_hid_sensor {
        public:
            iio_hid_sensor(const std::string& device_path, uint32_t frequency);

            ~iio_hid_sensor();

            // start capturing and polling.
            void start_capture(hid_callback sensor_callback);

            void stop_capture();

            const std::string& get_sensor_name() const { return _sensor_name; }

        private:
            void clear_buffer();

            void set_frequency(uint32_t frequency);

            void signal_stop();

            bool has_metadata();

            static bool set_fs_attribute(std::string path, int val);
            static bool sort_hids(hid_input* first, hid_input* second);

            void create_channel_array();

            // initialize the device sensor. reading its name and all of its inputs.
            void init(uint32_t frequency);

            // calculate the storage size of a scan
            uint32_t get_channel_size() const;

            // calculate the actual size of data
            uint32_t get_output_size() const;

            std::string get_sampling_frequency_name() const;

            // read the IIO device inputs.
            void read_device_inputs();

            // configure hid device via fd
            void write_integer_to_param(const std::string& param,int value);

            static const uint32_t buf_len = 128; // TODO
            int _stop_pipe_fd[2]; // write to _stop_pipe_fd[1] and read from _stop_pipe_fd[0]
            int _fd;
            int _iio_device_number;
            std::string _iio_device_path;
            std::string _sensor_name;
            std::string _sampling_frequency_name;
            std::list<hid_input*> _inputs;
            std::list<hid_input*> _channels;
            hid_callback _callback;
            std::atomic<bool> _is_capturing;
            std::unique_ptr<std::thread> _hid_thread;
            std::unique_ptr<std::thread> _pm_thread;    // Delayed initialization due to power-up sequence
        };

        class v4l_hid_device : public hid_device
        {
        public:
            v4l_hid_device(const hid_device_info& info);

            ~v4l_hid_device();

            void open(const std::vector<hid_profile>& hid_profiles);

            void close();

            std::vector<hid_sensor> get_sensors();

            void start_capture(hid_callback callback);

            void stop_capture();

            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                        const std::string& report_name,
                                                        custom_sensor_report_field report_field);

            static void foreach_hid_device(std::function<void(const hid_device_info&)> action);

        private:
            static bool get_hid_device_info(const char* dev_path, hid_device_info& device_info);

            std::vector<hid_profile> _hid_profiles;
            std::vector<hid_device_info> _hid_device_infos;
            std::vector<std::unique_ptr<iio_hid_sensor>> _iio_hid_sensors;
            std::vector<std::unique_ptr<hid_custom_sensor>> _hid_custom_sensors;
            std::vector<iio_hid_sensor*> _streaming_iio_sensors;
            std::vector<hid_custom_sensor*> _streaming_custom_sensors;
            static constexpr const char* custom_id{"custom"};
        };
    }
}
