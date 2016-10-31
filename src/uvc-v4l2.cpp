// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS_USE_V4L2_BACKEND

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
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>

#pragma GCC diagnostic ignored "-Wpedantic"
#include <libusb.h>
#pragma GCC diagnostic pop

#pragma GCC diagnostic ignored "-Woverflow"

namespace rsimpl
{
    namespace uvc
    {
        static void throw_error(const char * s)
        {
            std::ostringstream ss;
            ss << s << " error " << errno << ", " << strerror(errno);
            throw std::runtime_error(ss.str());
        }

        static void warn_error(const char * s)
        {
            LOG_ERROR(s << " error " << errno << ", " << strerror(errno));
        }

        static int xioctl(int fh, int request, void *arg)
        {
            int r=0;
            do {
                r = ioctl(fh, request, arg);
            } while (r < 0 && errno == EINTR);
            return r;
        }

        struct buffer { void * start; size_t length; };


        class v4l_usb_device : public usb_device
        {
        public:
            v4l_usb_device(const usb_device_info& info)
            {
                int status = libusb_init(&_usb_context);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

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

            ~v4l_usb_device()
            {
                if(_usb_device) libusb_unref_device(_usb_device);
                libusb_exit(_usb_context);
            }

            static void foreach_usb_device(libusb_context* usb_context, std::function<void(
                                                                const usb_device_info&,
                                                                libusb_device*)> action)
            {
                // Obtain libusb_device_handle for each device
                libusb_device ** list;
                int status = libusb_get_device_list(usb_context, &list);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));
                for(int i=0; list[i]; ++i)
                {
                    libusb_device * usb_device = list[i];
                    int busnum = libusb_get_bus_number(usb_device);
                    int devnum = libusb_get_device_address(usb_device);

                    auto parent_device = libusb_get_parent(usb_device);
                    if (parent_device)
                    {
                        int parent_devnum = libusb_get_device_address(libusb_get_parent(usb_device));

                        usb_device_info info;
                        std::stringstream ss;
                        ss << busnum << "/" << devnum << "/" << parent_devnum;
                        info.unique_id = ss.str();
                        action(info, usb_device);
                    }
                }
                libusb_free_device_list(list, 1);
            }

            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override
            {
                libusb_device_handle* usb_handle;
                int status = libusb_open(_usb_device, &usb_handle);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_open(...) returned " << libusb_error_name(status));

                status = libusb_claim_interface(usb_handle, _mi);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));

                int actual_length;
                status = libusb_bulk_transfer(usb_handle, 1, const_cast<uint8_t*>(data.data()), data.size(), &actual_length, timeout_ms);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                std::vector<uint8_t> result;

                std::this_thread::sleep_for(std::chrono::milliseconds(20));

                if (require_response)
                {
                    result.resize(1024);
                    status = libusb_bulk_transfer(usb_handle, 0x81, const_cast<uint8_t*>(result.data()), result.size(), &actual_length, timeout_ms);
                    if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
                    result.resize(actual_length);
                }

                libusb_close(usb_handle);

                return result;
            }

        private:
            libusb_context* _usb_context;
            libusb_device* _usb_device;
            int _mi;
        };

        class v4l_uvc_device : public uvc_device
        {
        public:
            static void foreach_uvc_device(
                    std::function<void(const uvc_device_info&,
                                       const std::string&)> action)
            {
                // Check if the uvcvideo kernel module is loaded
                std::ifstream modules("/proc/modules");
                std::string modulesline;
                std::regex regex("uvcvideo.* - Live.*");
                std::smatch match;
                bool module_found = false;


                while(std::getline(modules, modulesline) && !module_found)
                {
                    module_found = std::regex_match(modulesline, match, regex);
                }

                if(!module_found)
                {
                    throw std::runtime_error("uvcvideo kernel module is not loaded");
                }

                // Enumerate all subdevices present on the system
                DIR * dir = opendir("/sys/class/video4linux");
                if(!dir) throw std::runtime_error("Cannot access /sys/class/video4linux");
                while (dirent * entry = readdir(dir))
                {
                    std::string name = entry->d_name;
                    if(name == "." || name == "..") continue;

                    // Resolve a pathname to ignore virtual video devices
                    std::string path = "/sys/class/video4linux/" + name;
                    char buff[PATH_MAX];
                    ssize_t len = ::readlink(path.c_str(), buff, sizeof(buff)-1);
                    if (len != -1)
                    {
                        buff[len] = '\0';
                        std::string real_path = std::string(buff);
                        if (real_path.find("virtual") != std::string::npos)
                            continue;
                    }

                    try
                    {
                        int vid, pid, mi;
                        int busnum, devnum, parent_devnum;

                        auto dev_name = "/dev/" + name;

                        struct stat st;
                        if(stat(dev_name.c_str(), &st) < 0)
                        {
                            std::ostringstream ss; ss << "Cannot identify '" << dev_name << "': " << errno << ", " << strerror(errno);
                            throw std::runtime_error(ss.str());
                        }
                        if(!S_ISCHR(st.st_mode)) throw std::runtime_error(dev_name + " is no device");

                        // Search directory and up to three parent directories to find busnum/devnum
                        std::ostringstream ss; ss << "/sys/dev/char/" << major(st.st_rdev) << ":" << minor(st.st_rdev) << "/device/";
                        auto path = ss.str();
                        bool good = false;
                        for(int i=0; i<=3; ++i)
                        {
                            if(std::ifstream(path + "busnum") >> busnum)
                            {
                                if(std::ifstream(path + "devnum") >> devnum)
                                {
                                    if(std::ifstream(path + "../devnum") >> parent_devnum)
                                    {
                                        good = true;
                                        break;
                                    }
                                }
                            }
                            path += "../";
                        }
                        if(!good) throw std::runtime_error("Failed to read busnum/devnum");

                        std::string modalias;
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/modalias") >> modalias))
                            throw std::runtime_error("Failed to read modalias");
                        if(modalias.size() < 14 || modalias.substr(0,5) != "usb:v" || modalias[9] != 'p')
                            throw std::runtime_error("Not a usb format modalias");
                        if(!(std::istringstream(modalias.substr(5,4)) >> std::hex >> vid))
                            throw std::runtime_error("Failed to read vendor ID");
                        if(!(std::istringstream(modalias.substr(10,4)) >> std::hex >> pid))
                            throw std::runtime_error("Failed to read product ID");
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/bInterfaceNumber") >> std::hex >> mi))
                            throw std::runtime_error("Failed to read interface number");

                        uvc_device_info info;
                        info.pid = pid;
                        info.vid = vid;
                        info.mi = mi;
                        info.id = dev_name;
                        ss.str("");
                        ss << busnum << "/" << devnum << "/" << parent_devnum;
                        info.unique_id = ss.str();
                        action(info, dev_name);
                    }
                    catch(const std::exception & e)
                    {
                        LOG_INFO("Not a USB video device: " << e.what());
                    }
                }
                closedir(dir);
            }

            static uint32_t get_cid(rs_option option)
            {
                switch(option)
                {
                case RS_OPTION_BACKLIGHT_COMPENSATION: return V4L2_CID_BACKLIGHT_COMPENSATION;
                case RS_OPTION_BRIGHTNESS: return V4L2_CID_BRIGHTNESS;
                case RS_OPTION_CONTRAST: return V4L2_CID_CONTRAST;
                case RS_OPTION_EXPOSURE: return V4L2_CID_EXPOSURE_ABSOLUTE; // Is this actually valid? I'm getting a lot of VIDIOC error 22s...
                case RS_OPTION_GAIN: return V4L2_CID_GAIN;
                case RS_OPTION_GAMMA: return V4L2_CID_GAMMA;
                case RS_OPTION_HUE: return V4L2_CID_HUE;
                case RS_OPTION_SATURATION: return V4L2_CID_SATURATION;
                case RS_OPTION_SHARPNESS: return V4L2_CID_SHARPNESS;
                case RS_OPTION_WHITE_BALANCE: return V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                case RS_OPTION_ENABLE_AUTO_EXPOSURE: return V4L2_CID_EXPOSURE_AUTO; // Automatic gain/exposure control
                case RS_OPTION_ENABLE_AUTO_WHITE_BALANCE: return V4L2_CID_AUTO_WHITE_BALANCE;
                default: throw std::runtime_error(to_string() << "no v4l2 cid for option " << option);
                }
            }

            v4l_uvc_device(const uvc_device_info& info)
                : _name(""), _info(),
                  _is_capturing(false),
                  _is_alive(true),
                  _thread(nullptr)
            {
                foreach_uvc_device([&info, this](const uvc_device_info& i, const std::string& name)
                {
                    if (i == info)
                    {
                        _name = name;
                        _info = i;
                    }
                });
                if (_name == "") throw std::runtime_error("device is no longer connected!");


            }

            void capture_loop()
            {
                while(_is_capturing)
                {
                    poll();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            ~v4l_uvc_device()
            {
                _is_capturing = false;
                if (_thread) _thread->join();
            }

            void play(stream_profile profile, frame_callback callback) override
            {
                if(!_is_capturing && !_callback)
                {
                    _profile = profile;
                    _callback = callback;

                    v4l2_format fmt = {};
                    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    fmt.fmt.pix.width       = profile.width;
                    fmt.fmt.pix.height      = profile.height;
                    fmt.fmt.pix.pixelformat = (const big_endian<int> &)profile.format;
                    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
                    if(xioctl(_fd, VIDIOC_S_FMT, &fmt) < 0) throw_error("VIDIOC_S_FMT");

                    LOG_INFO("Trying to configure fourcc " << fourcc_to_string(fmt.fmt.pix.pixelformat));

                    v4l2_streamparm parm = {};
                    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_G_PARM, &parm) < 0) throw_error("VIDIOC_G_PARM");
                    parm.parm.capture.timeperframe.numerator = 1;
                    parm.parm.capture.timeperframe.denominator = profile.fps;
                    if(xioctl(_fd, VIDIOC_S_PARM, &parm) < 0) throw_error("VIDIOC_S_PARM");

                    // Init memory mapped IO
                    v4l2_requestbuffers req = {};
                    req.count = 4;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) throw std::runtime_error(_name + " does not support memory mapping");
                        else throw_error("VIDIOC_REQBUFS");
                    }
                    if(req.count < 2)
                    {
                        throw std::runtime_error("Insufficient buffer memory on " + _name);
                    }

                    _buffers.resize(req.count);
                    for(size_t i = 0; i < _buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(_fd, VIDIOC_QUERYBUF, &buf) < 0) throw_error("VIDIOC_QUERYBUF");

                        _buffers[i].length = buf.length;
                        _buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
                        if(_buffers[i].start == MAP_FAILED) throw_error("mmap");
                    }

                    // Start capturing
                    for(size_t i = 0; i < _buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                    }

                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    for(int i=0; i<10; ++i)
                    {
                        if(xioctl(_fd, VIDIOC_STREAMON, &type) < 0)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    }
                    if(xioctl(_fd, VIDIOC_STREAMON, &type) < 0) throw_error("VIDIOC_STREAMON");

                    _is_capturing = true;
                    _thread = std::unique_ptr<std::thread>(new std::thread([this](){ capture_loop(); }));
                }
            }

            void stop(stream_profile) override
            {
                if(_is_capturing)
                {
                    _is_capturing = false;
                    _thread->join();
                    _thread.reset();

                    // Stop streamining
                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0) warn_error("VIDIOC_STREAMOFF");

                    for(size_t i = 0; i < _buffers.size(); i++)
                    {
                        if(munmap(_buffers[i].start, _buffers[i].length) < 0) warn_error("munmap");
                    }

                    // Close memory mapped IO
                    struct v4l2_requestbuffers req = {};
                    req.count = 0;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) LOG_ERROR(_name + " does not support memory mapping");
                        else warn_error("VIDIOC_REQBUFS");
                    }

                    _callback = nullptr;
                }
            }

            std::string fourcc_to_string(uint32_t id) const
            {
                uint32_t device_fourcc = id;
                char fourcc_buff[sizeof(device_fourcc)+1];
                memcpy(fourcc_buff, &device_fourcc, sizeof(device_fourcc));
                fourcc_buff[sizeof(device_fourcc)] = 0;
                return fourcc_buff;
            }

            void poll()
            {
                int max_fd = _fd;
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_fd, &fds);

                struct timeval tv = {0,10000};
                if(select(max_fd + 1, &fds, NULL, NULL, &tv) < 0)
                {
                    if (errno == EINTR) return;
                    throw_error("select");
                }

                if(FD_ISSET(_fd, &fds))
                {
                    v4l2_buffer buf = {};
                    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buf.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                    {
                        if(errno == EAGAIN) return;
                        throw_error("VIDIOC_DQBUF");
                    }

                    frame_object fo { (int)_buffers[buf.index].length,
                                      _buffers[buf.index].start };

                    _callback(_profile, fo);

                    if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                }
            }

            void set_power_state(power_state state) override
            {
                if (state == D0 && _state == D3)
                {
                    _fd = open(_name.c_str(), O_RDWR | O_NONBLOCK, 0);
                    if(_fd < 0)
                    {
                        std::ostringstream ss; ss << "Cannot open '" << _name << "': " << errno << ", " << strerror(errno);
                        throw std::runtime_error(ss.str());
                    }

                    v4l2_capability cap = {};
                    if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
                    {
                        if(errno == EINVAL) throw std::runtime_error(_name + " is no V4L2 device");
                        else throw_error("VIDIOC_QUERYCAP");
                    }
                    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) throw std::runtime_error(_name + " is no video capture device");
                    if(!(cap.capabilities & V4L2_CAP_STREAMING)) throw std::runtime_error(_name + " does not support streaming I/O");

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
                    stop(_profile);
                    if(close(_fd) < 0) warn_error("close");
                    _fd = 0;
                }
                _state = state;
            }
            power_state get_power_state() const override { return _state; }

            void init_xu(const extension_unit& xu) override {}
            void set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size) override
            {
                uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_SET_CUR,
                                          static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_SET_CUR");
            }
            void get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override
            {
                uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_GET_CUR,
                                          static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_GET_CUR");
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override
            {
                control_range result;

                __u16 size = 0;
                __u16 value = 0; // all of the real sense extended controls are one byte,
                                // checking return value for UVC_GET_LEN and allocating
                                // appropriately might be better
                __u8 * data = (__u8 *)&value;
                struct uvc_xu_control_query xquery;
                memset(&xquery, 0, sizeof(xquery));
                xquery.query = UVC_GET_LEN;
                xquery.size = 2; // size seems to always be 2 for the LEN query, but
                                 //doesn't seem to be documented. Use result for size
                                 //in all future queries of the same control number
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = (__u8 *)&size;

                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_LEN");
                }

                xquery.query = UVC_GET_MIN;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MIN");
                }
                result.min = value;

                xquery.query = UVC_GET_MAX;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MAX");
                }
                result.max = value;

                xquery.query = UVC_GET_DEF;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_DEF");
                }
                result.def = value;

                xquery.query = UVC_GET_RES;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_CUR");
                }
                result.step = value;

                return result;
            }

            int get_pu(rs_option opt) const override
            {
                struct v4l2_control control = {get_cid(opt), 0};
                if (xioctl(_fd, VIDIOC_G_CTRL, &control) < 0) throw_error("VIDIOC_G_CTRL");
                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt)  { control.value = (V4L2_EXPOSURE_MANUAL==control.value) ? 0 : 1; }
                return control.value;
            }

            void set_pu(rs_option opt, int value) override
            {
                struct v4l2_control control = {get_cid(opt), value};
                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt) { control.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL; }
                if (xioctl(_fd, VIDIOC_S_CTRL, &control) < 0) throw_error("VIDIOC_S_CTRL");
            }

            control_range get_pu_range(rs_option option) const override
            {
                control_range range;

                // Auto controls range is trimed to {0,1} range
                if(option >= RS_OPTION_ENABLE_AUTO_EXPOSURE && option <= RS_OPTION_ENABLE_AUTO_WHITE_BALANCE)
                {
                    range.min  = 0;
                    range.max  = 1;
                    range.step = 1;
                    range.def  = 1;
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

                range.min  = query.minimum;
                range.max  = query.maximum;
                range.step = query.step;
                range.def  = query.default_value;

                return range;
            }

            std::vector<stream_profile> get_profiles() const override
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
                        LOG_WARNING("Unknown pixel-format " << pixel_format.description << "!");

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
                    else
                    {
                        LOG_WARNING("Recognized pixel-format " << pixel_format.description << "!");
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

                                    stream_profile p;
                                    p.format = fourcc;
                                    p.width = frame_size.discrete.width;
                                    p.height = frame_size.discrete.height;
                                    p.fps = fps;
                                    results.push_back(p);
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

            void lock() const override {}
            void unlock() const override {}

            std::string get_device_location() const override { return ""; }

        private:
            power_state _state;
            std::string _name;
            uvc_device_info _info;
            int _fd;
            std::vector<buffer> _buffers;
            stream_profile _profile;
            frame_callback _callback;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::unique_ptr<std::thread> _thread;
        };

        class v4l_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override
            {
                return std::make_shared<uvc::retry_controls_work_around>(
                        std::make_shared<v4l_uvc_device>(info));
            }
            std::vector<uvc_device_info> query_uvc_devices() const override
            {
                std::vector<uvc_device_info> results;
                v4l_uvc_device::foreach_uvc_device(
                [&results](const uvc_device_info& i, const std::string&)
                {
                    results.push_back(i);
                });
                return results;
            }

            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override
            {
                return std::make_shared<v4l_usb_device>(info);
            }
            std::vector<usb_device_info> query_usb_devices() const override
            {
                libusb_context * usb_context;
                int status = libusb_init(&usb_context);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

                std::vector<usb_device_info> results;
                v4l_usb_device::foreach_usb_device(usb_context,
                [&results](const usb_device_info& i, libusb_device* dev)
                {
                    results.push_back(i);
                });
                libusb_exit(usb_context);

                return results;
            }
        };

        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<v4l_backend>();
        }



        /*

        struct context
        {
            libusb_context * usb_context;

            context()
            {
                int status = libusb_init(&usb_context);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));
            }
            ~context()
            {
                libusb_exit(usb_context);
            }
        };

        struct subdevice
        {

        };

        struct device
        {
            const std::shared_ptr<context> parent;
            std::vector<std::unique_ptr<subdevice>> subdevices;
            std::thread thread;
            std::thread data_channel_thread;
            volatile bool stop;
            volatile bool data_stop;

            libusb_device * usb_device;
            libusb_device_handle * usb_handle;
            std::vector<int> claimed_interfaces;

            device(std::shared_ptr<context> parent) : parent(parent), stop(), data_stop(), usb_device(), usb_handle() {}
            ~device()
            {
                stop_streaming();

                for(auto interface_number : claimed_interfaces)
                {
                    int status = libusb_release_interface(usb_handle, interface_number);
                    if(status < 0) LOG_ERROR("libusb_release_interface(...) returned " << libusb_error_name(status));
                }

                if(usb_handle) libusb_close(usb_handle);
                if(usb_device) libusb_unref_device(usb_device);
            }

            bool has_mi(int mi) const
            {
                for(auto & sub : subdevices)
                {
                    if(sub->get_mi() == mi) return true;
                }
                return false;
            }

            void start_streaming()
            {
                std::vector<subdevice *> subs;

                for(auto & sub : subdevices)
                {
                    if(sub->callback)
                    {
                        sub->start_capture();
                        subs.push_back(sub.get());
                    }                
                }

                thread = std::thread([this, subs]()
                {
                    while(!stop) subdevice::poll(subs);
                });
            }

            void stop_streaming()
            {
                if(thread.joinable())
                {
                    stop = true;
                    thread.join();
                    stop = false;

                    for(auto & sub : subdevices) sub->stop_capture();
                }                
            }

            void start_data_acquisition()
            {
                std::vector<subdevice *> data_channel_subs;
                for (auto & sub : subdevices)
                {                   
                    if (sub->channel_data_callback)
                    {                        
                        data_channel_subs.push_back(sub.get());
                    }
                }
                
                // Motion events polling pipe
                if (claimed_interfaces.size())
                {
                    data_channel_thread = std::thread([this, data_channel_subs]()
                    {
                        // Polling 100ms timeout
                        while (!data_stop)
                        {
                            subdevice::poll_interrupts(this->usb_handle, data_channel_subs, 100);
                        }
                    });
                }
            }

            void stop_data_acquisition()
            {
                if (data_channel_thread.joinable())
                {
                    data_stop = true;
                    data_channel_thread.join();
                    data_stop = false;
                }
            }
        };

        ////////////
        // device //
        ////////////

        int get_vendor_id(const device & device) { return device.subdevices[0]->get_vid(); }
        int get_product_id(const device & device) { return device.subdevices[0]->get_pid(); }

        std::string get_usb_port_id(const device & device)
        {
            std::string usb_port = std::to_string(libusb_get_bus_number(device.usb_device)) + "-" +
                std::to_string(libusb_get_port_number(device.usb_device));
            return usb_port;
        }

        void get_control(const device & device, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            device.subdevices[xu.subdevice]->get_control(xu, ctrl, data, len);
        }
        void set_control(device & device, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            device.subdevices[xu.subdevice]->set_control(xu, ctrl, data, len);
        }

        void claim_interface(device & device, const guid & , int interface_number)
        {
            if(!device.usb_handle)
            {
                int status = libusb_open(device.usb_device, &device.usb_handle);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_open(...) returned " << libusb_error_name(status));
            }

            int status = libusb_claim_interface(device.usb_handle, interface_number);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));
            device.claimed_interfaces.push_back(interface_number);
        }
        void claim_aux_interface(device & device, const guid & interface_guid, int interface_number)
        {
            claim_interface(device, interface_guid, interface_number);
        }

        void bulk_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {
            if(!device.usb_handle) throw std::logic_error("called uvc::bulk_transfer before uvc::claim_interface");
            int status = libusb_bulk_transfer(device.usb_handle, endpoint, (unsigned char *)data, length, actual_length, timeout);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
        }

        void interrupt_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {
            if(!device.usb_handle) throw std::logic_error("called uvc::interrupt_transfer before uvc::claim_interface");
            int status = libusb_interrupt_transfer(device.usb_handle, endpoint, (unsigned char *)data, length, actual_length, timeout);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_interrupt_transfer(...) returned " << libusb_error_name(status));
        }

        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, video_channel_callback callback)
        {
            device.subdevices[subdevice_index]->set_format(width, height, (const big_endian<int> &)fourcc, fps, callback);
        }

        void set_subdevice_data_channel_handler(device & device, int subdevice_index, data_channel_callback callback)
        {
            device.subdevices[subdevice_index]->set_data_channel_cfg(callback);
        }

        void start_streaming(device & device, int )
        {
            device.start_streaming();
        }

        void stop_streaming(device & device)
        {
            device.stop_streaming();
        }       

        void start_data_acquisition(device & device)
        {
            device.start_data_acquisition();
        }

        void stop_data_acquisition(device & device)
        {
            device.stop_data_acquisition();
        }

        void set_pu_control(device & device, int subdevice, rs_option option, int value)
        {
            struct v4l2_control control = {get_cid(option), value};
            if (RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE==option) { control.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL; }
            if (xioctl(device.subdevices[subdevice]->fd, VIDIOC_S_CTRL, &control) < 0) throw_error("VIDIOC_S_CTRL");
        }

        int get_pu_control(const device & device, int subdevice, rs_option option)
        {
            struct v4l2_control control = {get_cid(option), 0};
            if (xioctl(device.subdevices[subdevice]->fd, VIDIOC_G_CTRL, &control) < 0) throw_error("VIDIOC_G_CTRL");
            if (RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE==option)  { control.value = (V4L2_EXPOSURE_MANUAL==control.value) ? 0 : 1; }
            return control.value;
        }

        void get_pu_control_range(const device & device, int subdevice, rs_option option, int * min, int * max, int * step, int * def)
        {
            // Auto controls range is trimed to {0,1} range
            if(option >= RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE && option <= RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE)
            {
                if(min)  *min  = 0;
                if(max)  *max  = 1;
                if(step) *step = 1;
                if(def)  *def  = 1;
                return;
            }

            struct v4l2_queryctrl query = {};
            query.id = get_cid(option);
            if (xioctl(device.subdevices[subdevice]->fd, VIDIOC_QUERYCTRL, &query) < 0)
            {
                // Some controls (exposure, auto exposure, auto hue) do not seem to work on V4L2
                // Instead of throwing an error, return an empty range. This will cause this control to be omitted on our UI sample.
                // TODO: Figure out what can be done about these options and make this work
                query.minimum = query.maximum = 0;
            }
            if(min)  *min  = query.minimum;
            if(max)  *max  = query.maximum;
            if(step) *step = query.step;
            if(def)  *def  = query.default_value;
        }

        void get_extension_control_range(const device & device, const extension_unit & xu, char control, int * min, int * max, int * step, int * def)
        {
            __u16 size = 0;
            __u8 value = 0; // all of the real sense extended controls are one byte,
                            // checking return value for UVC_GET_LEN and allocating
                            // appropriately might be better
            __u8 * data = (__u8 *)&value;
            struct uvc_xu_control_query xquery;
            memset(&xquery, 0, sizeof(xquery));
            xquery.query = UVC_GET_LEN;
            xquery.size = 2; // size seems to always be 2 for the LEN query, but
                             //doesn't seem to be documented. Use result for size
                             //in all future queries of the same control number
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = (__u8 *)&size;

            if(-1 == ioctl(device.subdevices[xu.subdevice]->fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_LEN");
            }

            xquery.query = UVC_GET_MIN;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = data;
            if(-1 == ioctl(device.subdevices[xu.subdevice]->fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MIN");
            }
            *min = value;

            xquery.query = UVC_GET_MAX;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = data;
            if(-1 == ioctl(device.subdevices[xu.subdevice]->fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MAX");
            }
            *max = value;

            xquery.query = UVC_GET_DEF;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = data;
            if(-1 == ioctl(device.subdevices[xu.subdevice]->fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_DEF");
            }
            *def = value;

            xquery.query = UVC_GET_RES;
            xquery.size = size;
            xquery.selector = control;
            xquery.unit = xu.unit;
            xquery.data = data;
            if(-1 == ioctl(device.subdevices[xu.subdevice]->fd,UVCIOC_CTRL_QUERY,&xquery)){
                throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_CUR");
            }
            *step = value;

        }
        /////////////
        // context //
        /////////////

        std::shared_ptr<context> create_context()
        {
            return std::make_shared<context>();
        }

        bool is_device_connected(device & device, int vid, int pid)
        {
            for (auto& sub : device.subdevices)
            {
                if (sub->vid == vid && sub->pid == pid)
                    return true;
            }

            return false;
        }

        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context)
        {
            // Check if the uvcvideo kernel module is loaded
            std::ifstream modules("/proc/modules");
            std::string modulesline;
            std::regex regex("uvcvideo.* - Live.*");
            std::smatch match;
            bool module_found = false;


            while(std::getline(modules,modulesline) && !module_found)
            {
                module_found = std::regex_match(modulesline, match, regex);
            }

            if(!module_found)
            {
                throw std::runtime_error("uvcvideo kernel module is not loaded");
            }


            // Enumerate all subdevices present on the system
            std::vector<std::unique_ptr<subdevice>> subdevices;
            DIR * dir = opendir("/sys/class/video4linux");
            if(!dir) throw std::runtime_error("Cannot access /sys/class/video4linux");
            while (dirent * entry = readdir(dir))
            {
                std::string name = entry->d_name;
                if(name == "." || name == "..") continue;

                // Resolve a pathname to ignore virtual video devices
                std::string path = "/sys/class/video4linux/" + name;
                char buff[PATH_MAX];
                ssize_t len = ::readlink(path.c_str(), buff, sizeof(buff)-1);
                if (len != -1) 
                {
                    buff[len] = '\0';
                    std::string real_path = std::string(buff);
                    if (real_path.find("virtual") != std::string::npos)
                        continue;
                }

                try
                {
                    std::unique_ptr<subdevice> sub(new subdevice(name));
                    subdevices.push_back(move(sub));
                }
                catch(const std::exception & e)
                {
                    LOG_INFO("Not a USB video device: " << e.what());
                }
            }
            closedir(dir);

            // Note: Subdevices of a given device may not be contiguous. We can test our grouping/sorting logic by calling random_shuffle.
            // std::random_shuffle(begin(subdevices), end(subdevices));

            // Group subdevices by busnum/devnum
            std::vector<std::shared_ptr<device>> devices;
            for(auto & sub : subdevices)
            {
                bool is_new_device = true;
                for(auto & dev : devices)
                {
                    if(sub->busnum == dev->subdevices[0]->busnum && sub->devnum == dev->subdevices[0]->devnum)
                    {
                        dev->subdevices.push_back(move(sub));
                        is_new_device = false;
                        break;
                    }
                }
                if(is_new_device)
                {
                    if (sub->vid == VID_INTEL_CAMERA && sub->pid == ZR300_FISHEYE_PID)  // avoid inserting fisheye camera as a device
                        continue;
                    devices.push_back(std::make_shared<device>(context));
                    devices.back()->subdevices.push_back(move(sub));
                }
            }

            // Sort subdevices within each device by multiple-interface index
            for(auto & dev : devices)
            {
                std::sort(begin(dev->subdevices), end(dev->subdevices), [](const std::unique_ptr<subdevice> & a, const std::unique_ptr<subdevice> & b)
                {
                    return a->mi < b->mi;
                });
            }


            // Insert fisheye camera as subDevice of ZR300
            for(auto & sub : subdevices)
            {
                if (!sub)
                    continue;

                for(auto & dev : devices)
                {
                    if (dev->subdevices[0]->vid == VID_INTEL_CAMERA && dev->subdevices[0]->pid == ZR300_CX3_PID && 
                        sub->vid == VID_INTEL_CAMERA && sub->pid == ZR300_FISHEYE_PID && dev->subdevices[0]->parent_devnum == sub->parent_devnum)
                    {
                        dev->subdevices.push_back(move(sub));
                        break;
                    }
                }
            }


            // Obtain libusb_device_handle for each device
            libusb_device ** list;
            int status = libusb_get_device_list(context->usb_context, &list);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));
            for(int i=0; list[i]; ++i)
            {
                libusb_device * usb_device = list[i];
                int busnum = libusb_get_bus_number(usb_device);
                int devnum = libusb_get_device_address(usb_device);

                // Look for a video device whose busnum/devnum matches this USB device
                for(auto & dev : devices)
                {
                    if (dev->subdevices.size() >=4)      // Make sure that four subdevices present
                    {
                        auto parent_device = libusb_get_parent(usb_device);
                        if (parent_device)
                        {
                            int parent_devnum = libusb_get_device_address(libusb_get_parent(usb_device));

                            // First, handle the special case of FishEye
                            bool bFishEyeDevice = ((busnum == dev->subdevices[3]->busnum) && (parent_devnum == dev->subdevices[3]->parent_devnum));
                            if(bFishEyeDevice && !dev->usb_device)
                            {
                                dev->usb_device = usb_device;
                                libusb_ref_device(usb_device);
                                break;
                            }
                        }
                    }

                    if(busnum == dev->subdevices[0]->busnum && devnum == dev->subdevices[0]->devnum)
                    {
                        if (!dev->usb_device) // do not override previous configuration
                        {
                            dev->usb_device = usb_device;
                            libusb_ref_device(usb_device);
                            break;
                        }
                    }                    
                }
            }
            libusb_free_device_list(list, 1);

            return devices;
        }*/
    }
}

#endif
