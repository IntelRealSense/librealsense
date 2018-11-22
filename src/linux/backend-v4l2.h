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
#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif
#pragma GCC diagnostic pop

// Metadata streaming nodes are available with kernels 4.16+
#ifdef V4L2_META_FMT_UVC
constexpr bool metadata_node = true;
#else
#pragma message ( "\nLibrealsense notification: V4L2_META_FMT_UVC was not defined, adding metadata constructs")

constexpr bool metadata_node = false;

// Providing missing parts from videodev2.h
// V4L2_META_FMT_UVC >> V4L2_CAP_META_CAPTURE is also defined, but the opposite does not hold
#define V4L2_META_FMT_UVC    v4l2_fourcc('U', 'V', 'C', 'H') /* UVC Payload Header */

#ifndef V4L2_CAP_META_CAPTURE
#define V4L2_CAP_META_CAPTURE    0x00800000  /* Specified in kernel header v4.16 */
#endif // V4L2_CAP_META_CAPTURE

#endif // V4L2_META_FMT_UVC

#ifndef V4L2_META_FMT_D4XX
#define V4L2_META_FMT_D4XX      v4l2_fourcc('D', '4', 'X', 'X') /* D400 Payload Header metadata */
#endif

// Use local definition of buf type to resolve for kernel versions
constexpr auto LOCAL_V4L2_BUF_TYPE_META_CAPTURE = (v4l2_buf_type)(13);

#pragma pack(push, 1)
// The struct definition is identical to uvc_meta_buf defined uvcvideo.h/ kernel 4.16 headers, and is provided to allow for cross-kernel compilation
struct uvc_meta_buffer {
    __u64 ns;               // system timestamp of the payload in nanoseconds
    __u16 sof;              // USB Frame Number
    __u8 length;            // length of the payload metadata header
    __u8 flags;             // payload header flags
    __u8* buf;              //device-specific metadata payload data
};
#pragma pack(pop)


namespace librealsense
{
    namespace platform
    {
        class named_mutex
        {
        public:
            named_mutex(const std::string& device_path, unsigned timeout);

            named_mutex(const named_mutex&) = delete;

            void lock();

            void unlock();

            bool try_lock();

        private:
            void acquire();
            void release();

            std::string _device_path;
            uint32_t _timeout;
            int _fildes;
            std::mutex _mutex;
        };

        static int xioctl(int fh, int request, void *arg);

        class buffer
        {
        public:
            buffer(int fd, v4l2_buf_type type, bool use_memory_map, int index);

            void prepare_for_streaming(int fd);

            ~buffer();

            void attach_buffer(const v4l2_buffer& buf);

            void detach_buffer();

            void request_next_frame(int fd);

            size_t get_full_length() const { return _length; }
            size_t get_length_frame_only() const { return _original_length; }

            uint8_t* get_frame_start() const { return _start; }

            bool use_memory_map() const { return _use_memory_map; }

        private:
            v4l2_buf_type _type;
            uint8_t* _start;
            size_t _length;
            size_t _original_length;
            bool _use_memory_map;
            int _index;
            v4l2_buffer _buf;
            std::mutex _mutex;
            bool _must_enqueue = false;
        };

        enum supported_kernel_buf_types : uint8_t
        {
            e_video_buf,
            e_metadata_buf,
            e_max_kernel_buf_type
        };


        // RAII handling of kernel buffers interchanges
        class buffers_mgr
        {
        public:
            buffers_mgr(bool memory_mapped_buf) :
                _md_start(nullptr),
                _md_size(0),
                _mmap_bufs(memory_mapped_buf)
                {};

            ~buffers_mgr(){};

            void    request_next_frame();
            void    handle_buffer(supported_kernel_buf_types buf_type, int file_desc,
                                   v4l2_buffer buf= v4l2_buffer(),
                                   std::shared_ptr<platform::buffer> data_buf=nullptr);

            uint8_t metadata_size() const { return _md_size; };
            void*   metadata_start() const { return _md_start; };

            void    set_md_attributes(uint8_t md_size, void* md_start)
                    { _md_start = md_start; _md_size = md_size; }

            void    set_md_from_video_node()
                    {
                        void* start = nullptr;
                        auto size = 0;

                        if (buffers.at(e_video_buf)._file_desc >=0)
                        {
                            auto buffer = buffers.at(e_video_buf)._data_buf;
                            start = buffer->get_frame_start() + buffer->get_length_frame_only();
                            size = (*(uint8_t*)start);
                        }
                        set_md_attributes(size,start);
                    }

        private:
            void*                               _md_start;  // marks the address of metadata blob
            uint8_t                             _md_size;   // metadata size is bounded by 255 bytes by design
            bool                                _mmap_bufs;

            // RAII for buffer exchange with kernel
            struct kernel_buf_guard
            {
                ~kernel_buf_guard()
                {
                    if (_data_buf && (!_managed))
                    {
                        //LOG_DEBUG("Enqueue buf " << _dq_buf.index << " for fd " << _file_desc);
                        if (xioctl(_file_desc, (int)VIDIOC_QBUF, &_dq_buf) < 0)
                        {
                            LOG_ERROR("xioctl(VIDIOC_QBUF) guard failed");
                        }
                    }
                }

                int                                 _file_desc=-1;
                bool                                _managed=false;
                std::shared_ptr<platform::buffer>   _data_buf=nullptr;
                v4l2_buffer                         _dq_buf{};
            };

            std::array<kernel_buf_guard, e_max_kernel_buf_type> buffers;
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

        class v4l_uvc_interface
        {
            virtual void capture_loop() = 0;

            virtual bool has_metadata() const = 0;

            virtual void streamon() const = 0;
            virtual void streamoff() const = 0;
            virtual void negotiate_kernel_buffers(size_t num) const = 0;

            virtual void allocate_io_buffers(size_t num) = 0;
            virtual void map_device_descriptor() = 0;
            virtual void unmap_device_descriptor() = 0;
            virtual void set_format(stream_profile profile) = 0;
            virtual void prepare_capture_buffers() = 0;
            virtual void stop_data_capture() = 0;
            virtual void acquire_metadata(buffers_mgr & buf_mgr,fd_set &fds) = 0;
        };

        class v4l_uvc_device : public uvc_device, public v4l_uvc_interface
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
            usb_spec get_usb_specification() const override { return _device_usb_spec; }

        protected:
            static uint32_t get_cid(rs2_option option);

            virtual void capture_loop();

            virtual bool has_metadata() const;

            virtual void streamon() const;
            virtual void streamoff() const;
            virtual void negotiate_kernel_buffers(size_t num) const;

            virtual void allocate_io_buffers(size_t num);
            virtual void map_device_descriptor();
            virtual void unmap_device_descriptor();
            virtual void set_format(stream_profile profile);
            virtual void prepare_capture_buffers();
            virtual void stop_data_capture();
            virtual void acquire_metadata(buffers_mgr & buf_mgr,fd_set &fds);

            power_state _state = D3;
            std::string _name = "";
            std::string _device_path = "";
            usb_spec _device_usb_spec = usb_undefined;
            uvc_device_info _info;

            std::vector<std::shared_ptr<buffer>> _buffers;
            stream_profile _profile;
            frame_callback _callback;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::atomic<bool> _is_started;
            std::unique_ptr<std::thread> _thread;
            std::unique_ptr<named_mutex> _named_mtx;
            bool _use_memory_map;
            int _max_fd = 0;                    // specifies the maximal pipe number the polling process will monitor
            std::vector<int>  _fds;             // list the file descriptors to be monitored during frames polling

        private:
            int _fd = 0;          // prevent unintentional abuse in derived class
            int _stop_pipe_fd[2]; // write to _stop_pipe_fd[1] and read from _stop_pipe_fd[0]

        };

        // Composition layer for uvc/metadata split nodes introduced with kernel 4.16
        class v4l_uvc_meta_device : public v4l_uvc_device
        {
        public:
            v4l_uvc_meta_device(const uvc_device_info& info, bool use_memory_map = false);

            ~v4l_uvc_meta_device();

        protected:

            void streamon() const;
            void streamoff() const;
            void negotiate_kernel_buffers(size_t num) const;
            void allocate_io_buffers(size_t num);
            void map_device_descriptor();
            void unmap_device_descriptor();
            void set_format(stream_profile profile);
            void prepare_capture_buffers();
            virtual void acquire_metadata(buffers_mgr & buf_mgr,fd_set &fds);

            int _md_fd = -1;
            std::string _md_name = "";

            std::vector<std::shared_ptr<buffer>> _md_buffers;
            stream_profile _md_profile;
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
