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

#pragma GCC diagnostic ignored "-Wpedantic"
#include "../third-party/libusb/libusb/libusb.h"
#pragma GCC diagnostic pop

namespace librealsense
{
    namespace platform
    {
        class named_mutex
        {
        public:
            named_mutex(const std::string& device_path, unsigned timeout);

            named_mutex(const named_mutex&) = delete;

            void lock() { acquire(); }
            void unlock() { release(); }

            bool try_lock();

            ~named_mutex();

        private:
            void acquire();

            void release();

            void create_named_mutex(const std::string& cam_id);

            void destroy_named_mutex();

            std::string _device_path;
            uint32_t _timeout;
            int _fildes;
        };

        static int xioctl(int fh, int request, void *arg);

        class buffer
        {
        public:
            buffer(int fd, bool use_memory_map, int index);

            void prepare_for_streaming(int fd);

            ~buffer();

            void attach_buffer(const v4l2_buffer& buf);

            void detach_buffer();

            void request_next_frame(int fd);

            size_t get_full_length() const { return _length; }
            size_t get_length_frame_only() const { return _original_length; }

            uint8_t* get_frame_start() const { return _start; }

        private:
            uint8_t* _start;
            size_t _length;
            size_t _original_length;
            bool _use_memory_map;
            int _index;
            v4l2_buffer _buf;
            std::mutex _mutex;
            bool _must_enqueue = false;
        };

        class v4l_usb_device : public usb_device
        {
        public:
            v4l_usb_device(const usb_device_info& info);

            ~v4l_usb_device();

            static void foreach_usb_device(libusb_context* usb_context, std::function<void(
                                                                const usb_device_info&,
                                                                libusb_device*)> action);

            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override;

        private:
            libusb_context* _usb_context;
            libusb_device* _usb_device = nullptr;
            int _mi;
        };

        class v4l_uvc_device : public uvc_device
        {
        public:
            static void foreach_uvc_device(
                    std::function<void(const uvc_device_info&,
                                       const std::string&)> action);

            v4l_uvc_device(const uvc_device_info& info, bool use_memory_map = false);

            ~v4l_uvc_device();

            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override;

            void stream_on(std::function<void(const notification& n)> error_handler) override;

            void start_callbacks() override;

            void stop_callbacks() override;

            void close(stream_profile) override;

            std::string fourcc_to_string(uint32_t id) const;

            void signal_stop();

            void poll();

            void set_power_state(power_state state) override;
            power_state get_power_state() const override { return _state; }

            void init_xu(const extension_unit& xu) override {}
            bool set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size) override;
            bool get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override;
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override;

            bool get_pu(rs2_option opt, int32_t& value) const override;

            bool set_pu(rs2_option opt, int32_t value) override;

            control_range get_pu_range(rs2_option option) const override;

            std::vector<stream_profile> get_profiles() const override;

            void lock() const override;
            void unlock() const override;

            std::string get_device_location() const override { return _device_path; }
        private:
            static uint32_t get_cid(rs2_option option);

            void capture_loop();

            bool has_metadata();

            power_state _state = D3;
            std::string _name;
            std::string _device_path;
            uvc_device_info _info;
            int _fd = 0;
            int _stop_pipe_fd[2]; // write to _stop_pipe_fd[1] and read from _stop_pipe_fd[0]

            std::vector<std::shared_ptr<buffer>> _buffers;
            stream_profile _profile;
            frame_callback _callback;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::atomic<bool> _is_started;
            std::unique_ptr<std::thread> _thread;
            std::unique_ptr<named_mutex> _named_mtx;
            bool _use_memory_map;
        };

        class v4l_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;

            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;

            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override;
            std::vector<hid_device_info> query_hid_devices() const override;

            std::shared_ptr<time_service> create_time_service() const override;
            std::shared_ptr<device_watcher> create_device_watcher() const;
        };
    }
}
