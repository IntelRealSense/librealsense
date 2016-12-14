// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS_USE_V4L2_BACKEND

#include "uvc.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <sstream>
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
            int r;
            do {
                r = ioctl(fh, request, arg);
            } while (r < 0 && errno == EINTR);
            return r;
        }

        struct buffer { void * start; size_t length; };

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
            std::string dev_name;   // Device name (typically of the form /dev/video*)
            int busnum, devnum, parent_devnum;     // USB device bus number and device number (needed for F200/SR300 direct USB controls)
            int vid, pid, mi;       // Vendor ID, product ID, and multiple interface index
            int fd;                 // File descriptor for this device
            std::vector<buffer> buffers;

            int width, height, format, fps;
            video_channel_callback callback = nullptr;
            data_channel_callback  channel_data_callback = nullptr;    // handle non-uvc data produced by device
            bool is_capturing;

            subdevice(const std::string & name) : dev_name("/dev/" + name), vid(), pid(), fd(), width(), height(), format(), callback(nullptr), channel_data_callback(nullptr), is_capturing()
            {
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

                fd = open(dev_name.c_str(), O_RDWR | O_NONBLOCK, 0);
                if(fd < 0)
                {
                    std::ostringstream ss; ss << "Cannot open '" << dev_name << "': " << errno << ", " << strerror(errno);
                    throw std::runtime_error(ss.str());
                }

                v4l2_capability cap = {};
                if(xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
                {
                    if(errno == EINVAL) throw std::runtime_error(dev_name + " is no V4L2 device");
                    else throw_error("VIDIOC_QUERYCAP");
                }
                if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) throw std::runtime_error(dev_name + " is no video capture device");
                if(!(cap.capabilities & V4L2_CAP_STREAMING)) throw std::runtime_error(dev_name + " does not support streaming I/O");

                // Select video input, video standard and tune here.
                v4l2_cropcap cropcap = {};
                cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0)
                {
                    v4l2_crop crop = {};
                    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    crop.c = cropcap.defrect; // reset to default
                    if(xioctl(fd, VIDIOC_S_CROP, &crop) < 0)
                    {
                        switch (errno)
                        {
                        case EINVAL: break; // Cropping not supported
                        default: break; // Errors ignored
                        }
                    }
                } else {} // Errors ignored
            }

            ~subdevice()
            {
                stop_capture();
                if(close(fd) < 0) warn_error("close");
            }

            int get_vid() const { return vid; }
            int get_pid() const { return pid; }
            int get_mi() const { return mi; }

            void get_control(const extension_unit & xu, uint8_t control, void * data, size_t size)
            {
            uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_GET_CUR, static_cast<uint16_t>(size), reinterpret_cast<uint8_t *>(data)};
                if(xioctl(fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_GET_CUR");
            }

            void set_control(const extension_unit & xu, uint8_t control, void * data, size_t size)
            {
            uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_SET_CUR, static_cast<uint16_t>(size), reinterpret_cast<uint8_t *>(data)};
                if(xioctl(fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_SET_CUR");
            }

            void set_format(int width, int height, int fourcc, int fps, video_channel_callback callback)
            {
                this->width = width;
                this->height = height;
                this->format = fourcc;
                this->fps = fps;
                this->callback = callback;
            }

            void set_data_channel_cfg(data_channel_callback callback)
            {                
                this->channel_data_callback = callback;
            }

            void start_capture()
            {
                if(!is_capturing)
                {
                    v4l2_format fmt = {};
                    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    fmt.fmt.pix.width       = width;
                    fmt.fmt.pix.height      = height;
                    fmt.fmt.pix.pixelformat = format;
                    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
                    if(xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) throw_error("VIDIOC_S_FMT");

                    v4l2_streamparm parm = {};
                    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(fd, VIDIOC_G_PARM, &parm) < 0) throw_error("VIDIOC_G_PARM");
                    parm.parm.capture.timeperframe.numerator = 1;
                    parm.parm.capture.timeperframe.denominator = fps;
                    if(xioctl(fd, VIDIOC_S_PARM, &parm) < 0) throw_error("VIDIOC_S_PARM");

                    // Init memory mapped IO
                    v4l2_requestbuffers req = {};
                    req.count = 4;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) throw std::runtime_error(dev_name + " does not support memory mapping");
                        else throw_error("VIDIOC_REQBUFS");
                    }
                    if(req.count < 2)
                    {
                        throw std::runtime_error("Insufficient buffer memory on " + dev_name);
                    }

                    buffers.resize(req.count);
                    for(size_t i = 0; i < buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) throw_error("VIDIOC_QUERYBUF");

                        buffers[i].length = buf.length;
                        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
                        if(buffers[i].start == MAP_FAILED) throw_error("mmap");
                    }

                    // Start capturing
                    for(size_t i = 0; i < buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                    }

                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    for(int i=0; i<10; ++i)
                    {
                        if(xioctl(fd, VIDIOC_STREAMON, &type) < 0)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    }
                    if(xioctl(fd, VIDIOC_STREAMON, &type) < 0) throw_error("VIDIOC_STREAMON");

                    is_capturing = true;
                }
            }

            void stop_capture()
            {
                if(is_capturing)
                {
                    // Stop streamining
                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(fd, VIDIOC_STREAMOFF, &type) < 0) warn_error("VIDIOC_STREAMOFF");

                    for(size_t i = 0; i < buffers.size(); i++)
                    {
                        if(munmap(buffers[i].start, buffers[i].length) < 0) warn_error("munmap");
                    }

                    // Close memory mapped IO
                    struct v4l2_requestbuffers req = {};
                    req.count = 0;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) LOG_ERROR(dev_name + " does not support memory mapping");
                        else warn_error("VIDIOC_REQBUFS");
                    }

                    callback = nullptr;
                    is_capturing = false;
                }
            }

            static void poll(const std::vector<subdevice *> & subdevices)
            {
                int max_fd = 0;
                fd_set fds;
                FD_ZERO(&fds);
                for(auto * sub : subdevices)
                {
                    FD_SET(sub->fd, &fds);
                    max_fd = std::max(max_fd, sub->fd);
                }

                struct timeval tv = {0,10000};
                if(select(max_fd + 1, &fds, NULL, NULL, &tv) < 0)
                {
                    if (errno == EINTR) return;
                    throw_error("select");
                }

                for(auto * sub : subdevices)
                {
                    if(FD_ISSET(sub->fd, &fds))
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        if(xioctl(sub->fd, VIDIOC_DQBUF, &buf) < 0)
                        {
                            if(errno == EAGAIN) return;
                            throw_error("VIDIOC_DQBUF");
                        }

                        sub->callback(sub->buffers[buf.index].start,
                                [sub, buf]() mutable {
                                    if(xioctl(sub->fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                                });
                    }
                }
            }            

            static void poll_interrupts(libusb_device_handle *handle, const std::vector<subdevice *> & subdevices, uint16_t timeout)
            {
                static const unsigned short interrupt_buf_size = 0x400;
                uint8_t buffer[interrupt_buf_size];                       /* 64 byte transfer buffer  - dedicated channel*/
                int num_bytes             = 0;                           /* Actual bytes transferred. */

                // TODO - replace hard-coded values : 0x82 and 1000
                int res = libusb_interrupt_transfer(handle, 0x84, buffer, interrupt_buf_size, &num_bytes, timeout);
                if (0 == res)
                {
                    // Propagate the data to device layer
                    for(auto & sub : subdevices)
                        if (sub->channel_data_callback)
                            sub->channel_data_callback(buffer, num_bytes);
                }
                else
                {
                    switch (res)
                    {
                    case LIBUSB_ERROR_TIMEOUT :
                        LOG_WARNING("interrupt e.p. timeout");
                        break;
                    default:
                        throw std::runtime_error(to_string() << "USB Interrupt end-point error " << libusb_strerror((libusb_error)res));
                        break;
                    }
                }
            }
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
            std::string usb_bus = std::to_string(libusb_get_bus_number(device.usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const int max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth];
            std::stringstream port_path;
            int port_count = libusb_get_port_numbers(device.usb_device, usb_ports, max_usb_depth);

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << "-" << std::to_string(usb_ports[i]);
            }

            return usb_bus + port_path.str();
        }

        void get_control(const device & device, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            device.subdevices[xu.subdevice]->get_control(xu, ctrl, data, len);
        }
        void set_control(device & device, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            device.subdevices[xu.subdevice]->set_control(xu, ctrl, data, len);
        }

        void claim_interface(device & device, const guid & /*interface_guid*/, int interface_number)
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

        void start_streaming(device & device, int /*num_transfer_bufs*/)
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

        static uint32_t get_cid(rs_option option)
        {
            switch(option)
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION: return V4L2_CID_BACKLIGHT_COMPENSATION;
            case RS_OPTION_COLOR_BRIGHTNESS: return V4L2_CID_BRIGHTNESS;
            case RS_OPTION_COLOR_CONTRAST: return V4L2_CID_CONTRAST;
            case RS_OPTION_COLOR_EXPOSURE: return V4L2_CID_EXPOSURE_ABSOLUTE; // Is this actually valid? I'm getting a lot of VIDIOC error 22s...
            case RS_OPTION_COLOR_GAIN: return V4L2_CID_GAIN;
            case RS_OPTION_COLOR_GAMMA: return V4L2_CID_GAMMA;
            case RS_OPTION_COLOR_HUE: return V4L2_CID_HUE;
            case RS_OPTION_COLOR_SATURATION: return V4L2_CID_SATURATION;
            case RS_OPTION_COLOR_SHARPNESS: return V4L2_CID_SHARPNESS;
            case RS_OPTION_COLOR_WHITE_BALANCE: return V4L2_CID_WHITE_BALANCE_TEMPERATURE;
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE: return V4L2_CID_EXPOSURE_AUTO; // Automatic gain/exposure control
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE: return V4L2_CID_AUTO_WHITE_BALANCE;
            case RS_OPTION_FISHEYE_GAIN: return V4L2_CID_GAIN;
            default: throw std::runtime_error(to_string() << "no v4l2 cid for option " << option);
            }
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
            __u8 value = 0; /* all of the real sense extended controls are one byte,
                            checking return value for UVC_GET_LEN and allocating
                            appropriately might be better */
            __u8 * data = (__u8 *)&value;
            struct uvc_xu_control_query xquery;
            memset(&xquery, 0, sizeof(xquery));
            xquery.query = UVC_GET_LEN;
            xquery.size = 2; /* size seems to always be 2 for the LEN query, but
                             doesn't seem to be documented. Use result for size
                             in all future queries of the same control number */
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
        }
    }
}

#endif
