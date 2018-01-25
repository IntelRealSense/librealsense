// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_V4L2_BACKEND

#include "backend-v4l2.h"
#include "backend-hid.h"
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

#include <sys/signalfd.h>
#include <signal.h>


#pragma GCC diagnostic ignored "-Woverflow"

const size_t MAX_DEV_PARENT_DIR = 10;


#ifdef ANDROID

#include <sys/sysmacros.h> // minor(...), major(...)

// https://android.googlesource.com/platform/bionic/+/master/libc/include/bits/lockf.h
#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3

// https://android.googlesource.com/platform/bionic/+/master/libc/bionic/lockf.cpp
int lockf64(int fd, int cmd, off64_t length)
{
    // Translate POSIX lockf into fcntl.
    struct flock64 fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_whence = SEEK_CUR;
    fl.l_start = 0;
    fl.l_len = length;

    if (cmd == F_ULOCK) {
        fl.l_type == F_UNLCK;
        cmd = F_SETLK64;
        return fcntl(fd, F_SETLK64, &fl);
    }

    if (cmd == F_LOCK) {
        fl.l_type = F_WRLCK;
        return fcntl(fd, F_SETLKW64, &fl);
    }

    if (cmd == F_TLOCK) {
        fl.l_type = F_WRLCK;
        return fcntl(fd, F_SETLK64, &fl);
    }
    if (cmd == F_TEST) {
        fl.l_type = F_RDLCK;
        if (fcntl(fd, F_GETLK64, &fl) == -1) return -1;
        if (fl.l_type == F_UNLCK || fl.l_pid == getpid()) return 0;
        errno = EACCES;
        return -1;
    }

    errno = EINVAL;
    return -1;
}

int lockf(int fd, int cmd, off_t length)
{
    return lockf64(fd, cmd, length);
}
#endif

namespace librealsense
{
    namespace platform
    {
        named_mutex::named_mutex(const std::string& device_path, unsigned timeout)
            : _device_path(device_path),
              _timeout(timeout) // TODO: try to lock with timeout
        {
            create_named_mutex(_device_path);
        }

        bool named_mutex::try_lock()
        {
            auto ret = lockf(_fildes, F_TLOCK, 0);
            if (ret != 0)
                return false;

            return true;
        }

        named_mutex::~named_mutex()
        {
            try{
                destroy_named_mutex();
            }
            catch(...)
            {

            }
        }

        void named_mutex::acquire()
        {
            auto ret = lockf(_fildes, F_LOCK, 0);
            if (ret != 0)
                throw linux_backend_exception(to_string() << "Acquire failed");
        }

        void named_mutex::release()
        {
            auto ret = lockf(_fildes, F_ULOCK, 0);
            if (ret != 0)
                throw linux_backend_exception(to_string() << "lockf(...) failed");
        }

        void named_mutex::create_named_mutex(const std::string& cam_id)
        {
            const auto max_retries = 10;

            for(auto i=0; i< max_retries; i++)
            {
                _fildes = open(cam_id.c_str(), O_RDWR);
                if (0 <= _fildes)
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (-1 == _fildes)
                throw linux_backend_exception(to_string() << "open(...) failed");
        }

        void named_mutex::destroy_named_mutex()
        {
            auto ret = close(_fildes);
            if (0 != ret)
                throw linux_backend_exception(to_string() << "close(...) failed");
        }

        static int xioctl(int fh, int request, void *arg)
        {
            int r=0;
            do {
                r = ioctl(fh, request, arg);
            } while (r < 0 && errno == EINTR);
            return r;
        }


        buffer::buffer(int fd, bool use_memory_map, int index)
            : _use_memory_map(use_memory_map), _index(index)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
            buf.index = index;
            if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
                throw linux_backend_exception("xioctl(VIDIOC_QUERYBUF) failed");

            _original_length = buf.length;
            _length = buf.length;
            if (use_memory_map)
            {
                _start = static_cast<uint8_t*>(mmap(NULL, buf.length,
                                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                                    fd, buf.m.offset));
                if(_start == MAP_FAILED)
                    linux_backend_exception("mmap failed");
            }
            else
            {
                _length += MAX_META_DATA_SIZE;
                _start = static_cast<uint8_t*>(malloc( buf.length + MAX_META_DATA_SIZE));
                if (!_start) throw linux_backend_exception("User_p allocation failed!");
                memset(_start, 0, _length);
            }
        }

        void buffer::prepare_for_streaming(int fd)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
            buf.index = _index;
            buf.length = _length;

            if ( !_use_memory_map )
            {
                buf.m.userptr = (unsigned long)_start;
            }
            if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                throw linux_backend_exception("xioctl(VIDIOC_QBUF) failed");
        }

        buffer::~buffer()
        {
            if (_use_memory_map)
            {
               if(munmap(_start, _length) < 0)
                   linux_backend_exception("munmap");
            }
            else
            {
               free(_start);
            }
        }

        void buffer::attach_buffer(const v4l2_buffer& buf)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _buf = buf;
            _must_enqueue = true;
        }

        void buffer::detach_buffer()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _must_enqueue = false;
        }

        void buffer::request_next_frame(int fd)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (_must_enqueue)
            {
                if (V4L2_MEMORY_USERPTR == _use_memory_map)
                {
                    auto metadata_offset = get_full_length() - MAX_META_DATA_SIZE;
                    memset((byte*)(get_frame_start()) + metadata_offset, 0, MAX_META_DATA_SIZE);
                }

                if (xioctl(fd, VIDIOC_QBUF, &_buf) < 0)
                    LOG_ERROR("xioctl(VIDIOC_QBUF) failed when requesting new frame! Last-error: " << strerror(errno));

                _must_enqueue = false;
            }
        }

        static std::string get_usb_port_id(libusb_device* usb_device)
        {
            auto usb_bus = std::to_string(libusb_get_bus_number(usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const auto max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth] = {};
            std::stringstream port_path;
            auto port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);
            auto usb_dev = std::to_string(libusb_get_device_address(usb_device));

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << std::to_string(usb_ports[i]) << (((i+1) < port_count)?".":"");
            }

            return usb_bus + "-" + port_path.str() + "-" + usb_dev;
        }

        v4l_usb_device::v4l_usb_device(const usb_device_info& info)
        {
            int status = libusb_init(&_usb_context);
            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));


            std::vector<usb_device_info> results;
            v4l_usb_device::foreach_usb_device(_usb_context,
            [&results, info, this](const usb_device_info& i, libusb_device* dev)
            {
                if (i.unique_id == info.unique_id)
                {
                    _usb_device = dev;
                    libusb_ref_device(dev);
                }
            });

            _mi = info.mi;
        }

        v4l_usb_device::~v4l_usb_device()
        {
            try
            {
                if(_usb_device)
                    libusb_unref_device(_usb_device);
                libusb_exit(_usb_context);
            }
            catch(...)
            {

            }
        }

        void v4l_usb_device::foreach_usb_device(libusb_context* usb_context, std::function<void(
                                                            const usb_device_info&,
                                                            libusb_device*)> action)
        {
            // Obtain libusb_device_handle for each device
            libusb_device ** list = nullptr;
            int status = libusb_get_device_list(usb_context, &list);

            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));

            for(int i=0; list[i]; ++i)
            {
                libusb_device * usb_device = list[i];
                libusb_config_descriptor *config;
                status = libusb_get_active_config_descriptor(usb_device, &config);
                if(status == 0)
                {
                    auto parent_device = libusb_get_parent(usb_device);
                    if (parent_device)
                    {
                        usb_device_info info{};
                        info.unique_id = get_usb_port_id(usb_device);
                        info.mi = config->bNumInterfaces - 1; // The hardware monitor USB interface is expected to be the last one
                        action(info, usb_device);
                    }
                    libusb_free_config_descriptor(config);
                }

            }
            libusb_free_device_list(list, 1);
        }

        std::vector<uint8_t> v4l_usb_device::send_receive(
            const std::vector<uint8_t>& data,
            int timeout_ms,
            bool require_response)
        {
            libusb_device_handle* usb_handle = nullptr;
            int status = libusb_open(_usb_device, &usb_handle);
            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_open(...) returned " << libusb_error_name(status));
            status = libusb_claim_interface(usb_handle, _mi);
            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));

            int actual_length;
            status = libusb_bulk_transfer(usb_handle, 1, const_cast<uint8_t*>(data.data()), data.size(), &actual_length, timeout_ms);
            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

            std::vector<uint8_t> result;


            if (require_response)
            {
                result.resize(1024);
                status = libusb_bulk_transfer(usb_handle, 0x81, const_cast<uint8_t*>(result.data()), result.size(), &actual_length, timeout_ms);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                result.resize(actual_length);
            }

            libusb_close(usb_handle);

            return result;
        }

        void v4l_uvc_device::foreach_uvc_device(
                std::function<void(const uvc_device_info&,
                                   const std::string&)> action)
        {
            // Enumerate all subdevices present on the system
            DIR * dir = opendir("/sys/class/video4linux");
            if(!dir)
            {
                LOG_ERROR("Cannot access /sys/class/video4linux");
                return;
            }
            while (dirent * entry = readdir(dir))
            {
                std::string name = entry->d_name;
                if(name == "." || name == "..") continue;

                // Resolve a pathname to ignore virtual video devices
                std::string path = "/sys/class/video4linux/" + name;
                char buff[PATH_MAX] = {0};
                if (realpath(path.c_str(), buff) != NULL)
                {
                    auto real_path = std::string(buff);
                    if (real_path.find("virtual") != std::string::npos)
                        continue;
                }

                try
                {
                    int vid, pid, mi;
                    std::string busnum, devnum, devpath;

                    auto dev_name = "/dev/" + name;

                    struct stat st = {};
                    if(stat(dev_name.c_str(), &st) < 0)
                    {
                        throw linux_backend_exception(to_string() << "Cannot identify '" << dev_name);
                    }
                    if(!S_ISCHR(st.st_mode))
                        throw linux_backend_exception(dev_name + " is no device");

                    // Search directory and up to three parent directories to find busnum/devnum
                    std::ostringstream ss; ss << "/sys/dev/char/" << major(st.st_rdev) << ":" << minor(st.st_rdev) << "/device/";
                    auto path = ss.str();
                    auto good = false;
                    for(auto i=0; i < MAX_DEV_PARENT_DIR; ++i)
                    {
                        if(std::ifstream(path + "busnum") >> busnum)
                        {
                            if(std::ifstream(path + "devnum") >> devnum)
                            {
                                if(std::ifstream(path + "devpath") >> devpath)
                                {
                                    good = true;
                                    break;
                                }
                            }
                        }
                        path += "../";
                    }
                    if(!good)
                    {
                        LOG_WARNING("Failed to read busnum/devnum. Device Path: " << path);
                        continue;
                    }

                    std::string modalias;
                    if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/modalias") >> modalias))
                        throw linux_backend_exception("Failed to read modalias");
                    if(modalias.size() < 14 || modalias.substr(0,5) != "usb:v" || modalias[9] != 'p')
                        throw linux_backend_exception("Not a usb format modalias");
                    if(!(std::istringstream(modalias.substr(5,4)) >> std::hex >> vid))
                        throw linux_backend_exception("Failed to read vendor ID");
                    if(!(std::istringstream(modalias.substr(10,4)) >> std::hex >> pid))
                        throw linux_backend_exception("Failed to read product ID");
                    if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/bInterfaceNumber") >> std::hex >> mi))
                        throw linux_backend_exception("Failed to read interface number");

                    uvc_device_info info{};
                    info.pid = pid;
                    info.vid = vid;
                    info.mi = mi;
                    info.id = dev_name;
                    info.device_path = std::string(buff);
                    info.unique_id = busnum + "-" + devpath + "-" + devnum;
                    action(info, dev_name);
                }
                catch(const std::exception & e)
                {
                    LOG_INFO("Not a USB video device: " << e.what());
                }
            }
            closedir(dir);
        }

        v4l_uvc_device::v4l_uvc_device(const uvc_device_info& info, bool use_memory_map)
            : _name(""), _info(),
              _is_capturing(false),
              _is_alive(true),
              _thread(nullptr),
              _use_memory_map(use_memory_map),
              _is_started(false),
              _stop_pipe_fd{}
        {
            foreach_uvc_device([&info, this](const uvc_device_info& i, const std::string& name)
            {
                if (i == info)
                {
                    _name = name;
                    _info = i;
                    _device_path = i.device_path;
                }
            });
            if (_name == "")
                throw linux_backend_exception("device is no longer connected!");

            _named_mtx = std::unique_ptr<named_mutex>(new named_mutex(_name, 5000));
        }

        v4l_uvc_device::~v4l_uvc_device()
        {
            _is_capturing = false;
            if (_thread) _thread->join();
        }

        void v4l_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback, int buffers)
        {
            if(!_is_capturing && !_callback)
            {
                v4l2_fmtdesc pixel_format = {};
                pixel_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                while (ioctl(_fd, VIDIOC_ENUM_FMT, &pixel_format) == 0)
                {
                    v4l2_frmsizeenum frame_size = {};
                    frame_size.pixel_format = pixel_format.pixelformat;

                    uint32_t fourcc = (const big_endian<int> &)pixel_format.pixelformat;

                    if (pixel_format.pixelformat == 0)
                    {
                        // Microsoft Depth GUIDs for R400 series are not yet recognized
                        // by the Linux kernel, but they do not require a patch, since there
                        // are "backup" Z16 and Y8 formats in place
                        static const std::set<std::string> pending_formats = {
                            "00000050-0000-0010-8000-00aa003",
                            "00000032-0000-0010-8000-00aa003",
                        };

                        if (std::find(pending_formats.begin(),
                                      pending_formats.end(),
                                      (const char*)pixel_format.description) ==
                            pending_formats.end())
                        {
                            const std::string s(to_string() << "!" << pixel_format.description);
                            std::regex rgx("!([0-9a-f]+)-.*");
                            std::smatch match;

                            if (std::regex_search(s.begin(), s.end(), match, rgx))
                            {
                                std::stringstream ss;
                                ss <<  match[1];
                                int id;
                                ss >> std::hex >> id;
                                fourcc = (const big_endian<int> &)id;

                                if (fourcc == profile.format)
                                {
                                    throw linux_backend_exception(to_string() << "The requested pixel format '"  << fourcc_to_string(id)
                                                                  << "' is not natively supported by the Linux kernel and likely requires a patch"
                                                                  <<  "!\nAlternatively please upgrade to kernel 4.12 or later.");
                                }
                            }
                        }
                    }
                    ++pixel_format.index;
                }


                v4l2_format fmt = {};
                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                fmt.fmt.pix.width       = profile.width;
                fmt.fmt.pix.height      = profile.height;
                fmt.fmt.pix.pixelformat = (const big_endian<int> &)profile.format;
                fmt.fmt.pix.field       = V4L2_FIELD_NONE;
                if(xioctl(_fd, VIDIOC_S_FMT, &fmt) < 0)
                {
                    throw linux_backend_exception("xioctl(VIDIOC_S_FMT) failed");
                }

                LOG_INFO("Trying to configure fourcc " << fourcc_to_string(fmt.fmt.pix.pixelformat));

                v4l2_streamparm parm = {};
                parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_G_PARM, &parm) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_G_PARM) failed");

                parm.parm.capture.timeperframe.numerator = 1;
                parm.parm.capture.timeperframe.denominator = profile.fps;
                if(xioctl(_fd, VIDIOC_S_PARM, &parm) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_S_PARM) failed");

                // Init memory mapped IO
                v4l2_requestbuffers req = {};
                req.count = buffers;
                req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                req.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
                if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                {
                    if(errno == EINVAL)
                        throw linux_backend_exception(_name + " does not support memory mapping");
                    else
                        throw linux_backend_exception("xioctl(VIDIOC_REQBUFS) failed");
                }

                for(size_t i = 0; i < buffers; ++i)
                {
                    _buffers.push_back(std::make_shared<buffer>(_fd, _use_memory_map, i));
                }

                _profile =  profile;
                _callback = callback;
            }
            else
            {
                throw wrong_api_call_sequence_exception("Device already streaming!");
            }
        }

        void v4l_uvc_device::stream_on(std::function<void(const notification& n)> error_handler)
        {
            if(!_is_capturing)
            {
                _error_handler = error_handler;

                // Start capturing
                for (auto&& buf : _buffers) buf->prepare_for_streaming(_fd);

                v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_STREAMON, &type) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_STREAMON) failed");

                _is_capturing = true;
                _thread = std::unique_ptr<std::thread>(new std::thread([this](){ capture_loop(); }));
            }
        }

        void v4l_uvc_device::start_callbacks()
        {
            _is_started = true;
        }

        void v4l_uvc_device::stop_callbacks()
        {
            _is_started = false;
        }

        void v4l_uvc_device::close(stream_profile)
        {
            if(_is_capturing)
            {
                _is_capturing = false;
                _is_started = false;
                signal_stop();

                _thread->join();
                _thread.reset();


                // Stop streamining
                v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_STREAMOFF) failed");
            }

            if (_callback)
            {
                for(size_t i = 0; i < _buffers.size(); i++)
                {
                    _buffers[i]->detach_buffer();
                }
                _buffers.resize(0);

                // Close memory mapped IO
                struct v4l2_requestbuffers req = {};
                req.count = 0;
                req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                req.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
                if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                {
                    if(errno == EINVAL)
                        LOG_ERROR(_name + " does not support memory mapping");
                    else
                        throw linux_backend_exception("xioctl(VIDIOC_REQBUFS) failed");
                }

                _callback = nullptr;
            }
        }

        std::string v4l_uvc_device::fourcc_to_string(uint32_t id) const
        {
            uint32_t device_fourcc = id;
            char fourcc_buff[sizeof(device_fourcc)+1];
            librealsense::copy(fourcc_buff, &device_fourcc, sizeof(device_fourcc));
            fourcc_buff[sizeof(device_fourcc)] = 0;
            return fourcc_buff;
        }

        void v4l_uvc_device::signal_stop()
        {
            char buff[1];
            buff[0] = 0;
            if (write(_stop_pipe_fd[1], buff, 1) < 0)
            {
                 throw linux_backend_exception("Could not signal video capture thread to stop. Error write to pipe.");
            }
        }

        void v4l_uvc_device::poll()
        {
            fd_set fds{};
            FD_ZERO(&fds);
            FD_SET(_fd, &fds);
            FD_SET(_stop_pipe_fd[0], &fds);
            FD_SET(_stop_pipe_fd[1], &fds);

            int max_fd = std::max(std::max(_stop_pipe_fd[0], _stop_pipe_fd[1]), _fd);

            struct timeval tv = {5,0};

            auto val = select(max_fd+1, &fds, NULL, NULL, &tv);

            if(val < 0)
            {
                _is_capturing = false;
                _is_started = false;
                signal_stop();

                _thread->join();
                _thread.reset();

                // Stop streamining
                v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_STREAMOFF) failed");
            }
            else if(val > 0)
            {
                if(FD_ISSET(_stop_pipe_fd[0], &fds) || FD_ISSET(_stop_pipe_fd[1], &fds))
                {
                    if(!_is_capturing)
                    {
                        LOG_INFO("Stream finished");
                        return;
                    }
                }
                else if(FD_ISSET(_fd, &fds))
                {
                    FD_ZERO(&fds);
                    FD_SET(_fd, &fds);
                    v4l2_buffer buf = {};
                    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buf.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
                    if(xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                    {
                        if(errno == EAGAIN)
                            return;

                        throw linux_backend_exception("xioctl(VIDIOC_DQBUF) failed");
                    }

                    bool moved_qbuff = false;
                    auto buffer = _buffers[buf.index];

                    if (_is_started)
                    {
                        if((buf.bytesused < buffer->get_full_length() - MAX_META_DATA_SIZE) &&
                                buf.bytesused > 0)
                        {
                            auto percentage = (100 * buf.bytesused) / buffer->get_full_length();
                            std::stringstream s;
                            s << "Incomplete frame detected!\nSize " << buf.bytesused
                              << " out of " << buffer->get_full_length() << " bytes (" << percentage << "%)";
                            librealsense::notification n = { RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED, 0, RS2_LOG_SEVERITY_WARN, s.str()};

                            _error_handler(n);
                        }
                        else
                        {
                            void* md_start = nullptr;
                            uint8_t md_size = 0;
                            if (has_metadata())
                            {
                                md_start = buffer->get_frame_start() + buffer->get_length_frame_only();
                                md_size = (*(uint8_t*)md_start);
                            }

                            frame_object fo{ buffer->get_length_frame_only(), md_size,
                                buffer->get_frame_start(), md_start };

                             if (buf.bytesused > 0)
                             {
                                 buffer->attach_buffer(buf);
                                 moved_qbuff = true;
                                 auto fd = _fd;
                                 _callback(_profile, fo,
                                           [fd, buffer]() mutable {
                                     buffer->request_next_frame(fd);
                                 });
                             }
                             else
                             {
                                 LOG_WARNING("Empty frame has arrived.");
                             }
                        }
                    }

                    if (!moved_qbuff)
                    {
                        if (xioctl(_fd, VIDIOC_QBUF, &buf) < 0)
                            throw linux_backend_exception("xioctl(VIDIOC_QBUF) failed");
                    }
                }
                else
                {
                    throw linux_backend_exception("FD_ISSET returned false");
                }
            }
            else
            {
                LOG_WARNING("Frames didn't arrived within 5 seconds");
                librealsense::notification n = {RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT, 0, RS2_LOG_SEVERITY_WARN,  "Frames didn't arrived within 5 seconds"};

                _error_handler(n);
            }
        }

        void v4l_uvc_device::set_power_state(power_state state)
        {
            if (state == D0 && _state == D3)
            {
                _fd = open(_name.c_str(), O_RDWR | O_NONBLOCK, 0);
                if(_fd < 0)
                    throw linux_backend_exception(to_string() << "Cannot open '" << _name);


                if (pipe(_stop_pipe_fd) < 0)
                    throw linux_backend_exception("v4l_uvc_device: Cannot create pipe!");

                v4l2_capability cap = {};
                if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
                {
                    if(errno == EINVAL)
                        throw linux_backend_exception(_name + " is no V4L2 device");
                    else
                        throw linux_backend_exception("xioctl(VIDIOC_QUERYCAP) failed");
                }
                if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
                    throw linux_backend_exception(_name + " is no video capture device");

                if(!(cap.capabilities & V4L2_CAP_STREAMING))
                    throw linux_backend_exception(_name + " does not support streaming I/O");

                // Select video input, video standard and tune here.
                v4l2_cropcap cropcap = {};
                cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_CROPCAP, &cropcap) == 0)
                {
                    v4l2_crop crop = {};
                    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    crop.c = cropcap.defrect; // reset to default
                    if(xioctl(_fd, VIDIOC_S_CROP, &crop) < 0)
                    {
                        switch (errno)
                        {
                        case EINVAL: break; // Cropping not supported
                        default: break; // Errors ignored
                        }
                    }
                } else {} // Errors ignored
            }
            if (state == D3 && _state == D0)
            {
                close(_profile);
                if(::close(_fd) < 0)
                    throw linux_backend_exception("v4l_uvc_device: close(_fd) failed");

                if(::close(_stop_pipe_fd[0]) < 0)
                   throw linux_backend_exception("v4l_uvc_device: close(_stop_pipe_fd[0]) failed");
                if(::close(_stop_pipe_fd[1]) < 0)
                   throw linux_backend_exception("v4l_uvc_device: close(_stop_pipe_fd[1]) failed");

                _fd = 0;
                _stop_pipe_fd[0] = _stop_pipe_fd[1] = 0;
            }
            _state = state;
        }

        bool v4l_uvc_device::set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size)
        {
            uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_SET_CUR,
                                      static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
            if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0)
            {
                if (errno == EIO || errno == EAGAIN) // TODO: Log?
                    return false;

                throw linux_backend_exception("set_xu(...). xioctl(UVCIOC_CTRL_QUERY) failed");
            }

            return true;
        }
        bool v4l_uvc_device::get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const
        {
            uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_GET_CUR,
                                      static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
            if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0)
            {
                if (errno == EIO || errno == EAGAIN) // TODO: Log?
                    return false;

                throw linux_backend_exception("get_xu(...). xioctl(UVCIOC_CTRL_QUERY) failed");
            }

            return true;
        }
        control_range v4l_uvc_device::get_xu_range(const extension_unit& xu, uint8_t control, int len) const
        {
            control_range result{};
            __u16 size = 0;
            //__u32 value = 0;      // all of the real sense extended controls are up to 4 bytes
                                    // checking return value for UVC_GET_LEN and allocating
                                    // appropriately might be better
            //__u8 * data = (__u8 *)&value;
            // MS XU controls are partially supported only
            struct uvc_xu_control_query xquery = {};
            memset(&xquery, 0, sizeof(xquery));
            xquery.query = UVC_GET_LEN;
            xquery.size = 2; // size seems to always be 2 for the LEN query, but
                             //doesn't seem to be documented. Use result for size
                             //in all future queries of the same control number
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = (__u8 *)&size;

            if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw linux_backend_exception("xioctl(UVC_GET_LEN) failed");
            }

            assert(size<=len);

            std::vector<uint8_t> buf;
            auto buf_size = std::max((size_t)len,sizeof(__u32));
            buf.resize(buf_size);

            xquery.query = UVC_GET_MIN;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = buf.data();
            if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw linux_backend_exception("xioctl(UVC_GET_MIN) failed");
            }
            result.min.resize(buf_size);
            std::copy(buf.begin(), buf.end(), result.min.begin());

            xquery.query = UVC_GET_MAX;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = buf.data();
            if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw linux_backend_exception("xioctl(UVC_GET_MAX) failed");
            }
            result.max.resize(buf_size);
            std::copy(buf.begin(), buf.end(), result.max.begin());

            xquery.query = UVC_GET_DEF;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = buf.data();
            if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw linux_backend_exception("xioctl(UVC_GET_DEF) failed");
            }
            result.def.resize(buf_size);
            std::copy(buf.begin(), buf.end(), result.def.begin());

            xquery.query = UVC_GET_RES;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = buf.data();
            if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw linux_backend_exception("xioctl(UVC_GET_CUR) failed");
            }
            result.step.resize(buf_size);
            std::copy(buf.begin(), buf.end(), result.step.begin());

           return result;
        }

        bool v4l_uvc_device::get_pu(rs2_option opt, int32_t& value) const
        {
            struct v4l2_control control = {get_cid(opt), 0};
            if (xioctl(_fd, VIDIOC_G_CTRL, &control) < 0)
            {
                if (errno == EIO || errno == EAGAIN) // TODO: Log?
                    return false;

                throw linux_backend_exception("xioctl(VIDIOC_G_CTRL) failed");
            }

            if (RS2_OPTION_ENABLE_AUTO_EXPOSURE==opt)  { control.value = (V4L2_EXPOSURE_MANUAL==control.value) ? 0 : 1; }
            value = control.value;

            return true;
        }

        bool v4l_uvc_device::set_pu(rs2_option opt, int32_t value)
        {
            struct v4l2_control control = {get_cid(opt), value};
            if (RS2_OPTION_ENABLE_AUTO_EXPOSURE==opt) { control.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL; }
            if (xioctl(_fd, VIDIOC_S_CTRL, &control) < 0)
            {
                if (errno == EIO || errno == EAGAIN) // TODO: Log?
                    return false;

                throw linux_backend_exception("xioctl(VIDIOC_S_CTRL) failed");
            }

            return true;
        }

        control_range v4l_uvc_device::get_pu_range(rs2_option option) const
        {
            // Auto controls range is trimed to {0,1} range
            if(option >= RS2_OPTION_ENABLE_AUTO_EXPOSURE && option <= RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)
            {
                static const int32_t min = 0, max = 1, step = 1, def = 1;
                control_range range(min, max, step, def);

                return range;
            }

            struct v4l2_queryctrl query = {};
            query.id = get_cid(option);
            if (xioctl(_fd, VIDIOC_QUERYCTRL, &query) < 0)
            {
                // Some controls (exposure, auto exposure, auto hue) do not seem to work on V4L2
                // Instead of throwing an error, return an empty range. This will cause this control to be omitted on our UI sample.
                // TODO: Figure out what can be done about these options and make this work
                query.minimum = query.maximum = 0;
            }

            control_range range(query.minimum, query.maximum, query.step, query.default_value);

            return range;
        }

        std::vector<stream_profile> v4l_uvc_device::get_profiles() const
        {
            std::vector<stream_profile> results;

            // Retrieve the caps one by one, first get pixel format, then sizes, then
            // frame rates. See http://linuxtv.org/downloads/v4l-dvb-apis for reference.
            v4l2_fmtdesc pixel_format = {};
            pixel_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while (ioctl(_fd, VIDIOC_ENUM_FMT, &pixel_format) == 0)
            {
                v4l2_frmsizeenum frame_size = {};
                frame_size.pixel_format = pixel_format.pixelformat;

                uint32_t fourcc = (const big_endian<int> &)pixel_format.pixelformat;

                if (pixel_format.pixelformat == 0)
                {
                    // Microsoft Depth GUIDs for R400 series are not yet recognized
                    // by the Linux kernel, but they do not require a patch, since there
                    // are "backup" Z16 and Y8 formats in place
                    std::set<std::string> known_problematic_formats = {
                        "00000050-0000-0010-8000-00aa003",
                        "00000032-0000-0010-8000-00aa003",
                    };

                    if (std::find(known_problematic_formats.begin(),
                                  known_problematic_formats.end(),
                                  (const char*)pixel_format.description) ==
                        known_problematic_formats.end())
                    {
                        const std::string s(to_string() << "!" << pixel_format.description);
                        std::regex rgx("!([0-9a-f]+)-.*");
                        std::smatch match;

                        if (std::regex_search(s.begin(), s.end(), match, rgx))
                        {
                            std::stringstream ss;
                            ss <<  match[1];
                            int id;
                            ss >> std::hex >> id;
                            fourcc = (const big_endian<int> &)id;

                            auto format_str = fourcc_to_string(id);
                            LOG_WARNING("Pixel format " << pixel_format.description << " likely requires patch for fourcc code " << format_str << "!");
                        }
                    }
                }
                else
                {
                    LOG_DEBUG("Recognized pixel-format " << pixel_format.description);
                }

                while (ioctl(_fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) == 0)
                {
                    v4l2_frmivalenum frame_interval = {};
                    frame_interval.pixel_format = pixel_format.pixelformat;
                    frame_interval.width = frame_size.discrete.width;
                    frame_interval.height = frame_size.discrete.height;
                    while (ioctl(_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frame_interval) == 0)
                    {
                        if (frame_interval.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                        {
                            if (frame_interval.discrete.numerator != 0)
                            {
                                auto fps =
                                    static_cast<float>(frame_interval.discrete.denominator) /
                                    static_cast<float>(frame_interval.discrete.numerator);

                                stream_profile p{};
                                p.format = fourcc;
                                p.width = frame_size.discrete.width;
                                p.height = frame_size.discrete.height;
                                p.fps = fps;
                                if (fourcc != 0) results.push_back(p);
                            }
                        }

                        ++frame_interval.index;
                    }

                     ++frame_size.index;
                }

                ++pixel_format.index;
            }
            return results;
        }

        void v4l_uvc_device::lock() const
        {
            _named_mtx->lock();
        }
        void v4l_uvc_device::unlock() const
        {
            _named_mtx->unlock();
        }

        uint32_t v4l_uvc_device::get_cid(rs2_option option)
        {
            switch(option)
            {
            case RS2_OPTION_BACKLIGHT_COMPENSATION: return V4L2_CID_BACKLIGHT_COMPENSATION;
            case RS2_OPTION_BRIGHTNESS: return V4L2_CID_BRIGHTNESS;
            case RS2_OPTION_CONTRAST: return V4L2_CID_CONTRAST;
            case RS2_OPTION_EXPOSURE: return V4L2_CID_EXPOSURE_ABSOLUTE; // Is this actually valid? I'm getting a lot of VIDIOC error 22s...
            case RS2_OPTION_GAIN: return V4L2_CID_GAIN;
            case RS2_OPTION_GAMMA: return V4L2_CID_GAMMA;
            case RS2_OPTION_HUE: return V4L2_CID_HUE;
            case RS2_OPTION_SATURATION: return V4L2_CID_SATURATION;
            case RS2_OPTION_SHARPNESS: return V4L2_CID_SHARPNESS;
            case RS2_OPTION_WHITE_BALANCE: return V4L2_CID_WHITE_BALANCE_TEMPERATURE;
            case RS2_OPTION_ENABLE_AUTO_EXPOSURE: return V4L2_CID_EXPOSURE_AUTO; // Automatic gain/exposure control
            case RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE: return V4L2_CID_AUTO_WHITE_BALANCE;
            case RS2_OPTION_POWER_LINE_FREQUENCY : return V4L2_CID_POWER_LINE_FREQUENCY;
            case RS2_OPTION_AUTO_EXPOSURE_PRIORITY: return V4L2_CID_EXPOSURE_AUTO_PRIORITY;
            default: throw linux_backend_exception(to_string() << "no v4l2 cid for option " << option);
            }
        }

        void v4l_uvc_device::capture_loop()
        {
            try
            {
                while(_is_capturing)
                {
                    poll();
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR(ex.what());

                librealsense::notification n = {RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_ERROR, ex.what()};

                _error_handler(n);
            }
        }

        bool v4l_uvc_device::has_metadata()
        {
            return !_use_memory_map;
        }

        std::shared_ptr<uvc_device> v4l_backend::create_uvc_device(uvc_device_info info) const
        {
            return std::make_shared<platform::retry_controls_work_around>(
                    std::make_shared<v4l_uvc_device>(info));
        }
        std::vector<uvc_device_info> v4l_backend::query_uvc_devices() const
        {
            std::vector<uvc_device_info> results;
            v4l_uvc_device::foreach_uvc_device(
            [&results](const uvc_device_info& i, const std::string&)
            {
                results.push_back(i);
            });
            return results;
        }

        std::shared_ptr<usb_device> v4l_backend::create_usb_device(usb_device_info info) const
        {
            return std::make_shared<v4l_usb_device>(info);
        }
        std::vector<usb_device_info> v4l_backend::query_usb_devices() const
        {
            libusb_context * usb_context = nullptr;
            int status = libusb_init(&usb_context);
            if(status < 0)
                throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

            std::vector<usb_device_info> results;
            v4l_usb_device::foreach_usb_device(usb_context,
            [&results](const usb_device_info& i, libusb_device* dev)
            {
                results.push_back(i);
            });
            libusb_exit(usb_context);

            return results;
        }

        std::shared_ptr<hid_device> v4l_backend::create_hid_device(hid_device_info info) const
        {
            return std::make_shared<v4l_hid_device>(info);
        }

        std::vector<hid_device_info> v4l_backend::query_hid_devices() const
        {
            std::vector<hid_device_info> results;
            v4l_hid_device::foreach_hid_device([&](const hid_device_info& hid_dev_info){
                results.push_back(hid_dev_info);
            });

            return results;
        }
        std::shared_ptr<time_service> v4l_backend::create_time_service() const
        {
            return std::make_shared<os_time_service>();
        }

        std::shared_ptr<device_watcher> v4l_backend::create_device_watcher() const
        {
            return std::make_shared<polling_device_watcher>(this);
        }

        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<v4l_backend>();
        }
    }
}

#endif
