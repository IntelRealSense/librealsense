// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "backend-v4l2.h"
#include "backend-hid.h"
#include "backend.h"
#include "types.h"
#include "usb/usb-enumerator.h"
#include "usb/usb-device.h"

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
#include <sys/sysmacros.h> // minor(...), major(...)
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

#include "../tm2/tm-boot.h"

//#define DEBUG_V4L
#ifdef DEBUG_V4L
#define LOG_DEBUG_V4L(...)   do { CLOG(DEBUG   ,"librealsense") << __VA_ARGS__; } while(false)
#else
#define LOG_DEBUG_V4L(...)
#endif //DEBUG_V4L


#ifdef ANDROID

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
        fl.l_type = F_UNLCK;
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
              _timeout(timeout), // TODO: try to lock with timeout
              _fildes(-1)
        {
        }

        named_mutex::~named_mutex()
        {
            unlock();
        }

        void named_mutex::lock()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            acquire();
        }

        void named_mutex::unlock()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            release();
        }

        bool named_mutex::try_lock()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (-1 == _fildes)
            {
                _fildes = open(_device_path.c_str(), O_RDWR, 0); //TODO: check
                if(_fildes < 0)
                    return false;
            }

            auto ret = lockf(_fildes, F_TLOCK, 0);
            if (ret != 0)
                return false;

            return true;
        }

        void named_mutex::acquire()
        {
            if (-1 == _fildes)
            {
                _fildes = open(_device_path.c_str(), O_RDWR, 0); //TODO: check
                if(0 > _fildes)
                    throw linux_backend_exception(to_string() << "Cannot open '" << _device_path);
            }

            auto ret = lockf(_fildes, F_LOCK, 0);
            if (0 != ret)
                throw linux_backend_exception(to_string() << "Acquire failed");
        }

        void named_mutex::release()
        {
            if (-1 == _fildes)
                return;

            auto ret = lockf(_fildes, F_ULOCK, 0);
            if (0 != ret)
                throw linux_backend_exception(to_string() << "lockf(...) failed");

            ret = close(_fildes);
            if (0 != ret)
                throw linux_backend_exception(to_string() << "close(...) failed");

            _fildes = -1;
        }

        static int xioctl(int fh, unsigned long request, void *arg)
        {
            int r=0;
            do {
                r = ioctl(fh, request, arg);
            } while (r < 0 && errno == EINTR);
            return r;
        }

        buffer::buffer(int fd, v4l2_buf_type type, bool use_memory_map, uint32_t index)
            : _type(type), _use_memory_map(use_memory_map), _index(index)
        {
            v4l2_buffer buf = {};
            buf.type = _type;
            buf.memory = use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
            buf.index = index;
            if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
                throw linux_backend_exception("xioctl(VIDIOC_QUERYBUF) failed");

            // Prior to kernel 4.16 metadata payload was attached to the end of the video payload
            uint8_t md_extra = (V4L2_BUF_TYPE_VIDEO_CAPTURE==type) ? MAX_META_DATA_SIZE : 0;
            _original_length = buf.length;
            _length = _original_length + md_extra;

            if (use_memory_map)
            {
                _start = static_cast<uint8_t*>(mmap(nullptr, buf.length,
                                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                                    fd, buf.m.offset));
                if(_start == MAP_FAILED)
                    throw linux_backend_exception("mmap failed");
            }
            else
            {
                //_length += (V4L2_BUF_TYPE_VIDEO_CAPTURE==type) ? MAX_META_DATA_SIZE : 0;
                _start = static_cast<uint8_t*>(malloc( _length));
                if (!_start) throw linux_backend_exception("User_p allocation failed!");
                memset(_start, 0, _length);
            }
        }

        void buffer::prepare_for_streaming(int fd)
        {
            v4l2_buffer buf = {};
            buf.type = _type;
            buf.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
            buf.index = _index;
            buf.length = _length;

            if ( !_use_memory_map )
            {
                buf.m.userptr = reinterpret_cast<unsigned long>(_start);
            }
            if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                throw linux_backend_exception("xioctl(VIDIOC_QBUF) failed");
            else
                LOG_DEBUG_V4L("prepare_for_streaming fd " << std::dec << fd);
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
                if (!_use_memory_map)
                {
                    auto metadata_offset = get_full_length() - MAX_META_DATA_SIZE;
                    memset((byte*)(get_frame_start()) + metadata_offset, 0, MAX_META_DATA_SIZE);
                }

                LOG_DEBUG_V4L("Enqueue buf " << std::dec << _buf.index << " for fd " << fd);
                if (xioctl(fd, VIDIOC_QBUF, &_buf) < 0)
                {
                    LOG_ERROR("xioctl(VIDIOC_QBUF) failed when requesting new frame! fd: " << fd << " error: " << strerror(errno));
                }

                _must_enqueue = false;
            }
        }

        void buffers_mgr::handle_buffer(supported_kernel_buf_types  buf_type,
                                           int                      file_desc,
                                           v4l2_buffer              v4l_buf,
                                           std::shared_ptr<platform::buffer> data_buf
                                           )
        {
            if (e_max_kernel_buf_type <=buf_type)
                throw linux_backend_exception("invalid kernel buffer type request");

            if (file_desc < 1)
            {
                // QBUF to be performed by a 3rd party
                this->buffers.at(buf_type)._managed  = true;
            }
            else
            {
                buffers.at(buf_type)._file_desc = file_desc;
                buffers.at(buf_type)._managed = false;
                buffers.at(buf_type)._data_buf = data_buf;
                buffers.at(buf_type)._dq_buf = v4l_buf;
            }
        }

        void buffers_mgr::request_next_frame()
        {
            for (auto& buf : buffers)
            {
                if (buf._data_buf && (buf._file_desc >= 0))
                    buf._data_buf->request_next_frame(buf._file_desc);
            };
        }

        void buffers_mgr::set_md_from_video_node(bool compressed)
        {
            void* md_start = nullptr;
            auto md_size = 0;

            if (buffers.at(e_video_buf)._file_desc >=0)
            {
                static const int d4xx_md_size = 248;
                auto buffer = buffers.at(e_video_buf)._data_buf;
                auto dq  = buffers.at(e_video_buf)._dq_buf;
                auto fr_payload_size = buffer->get_length_frame_only();

                // For compressed data assume D4XX metadata struct
                // TODO - devise SKU-agnostic heuristics
                auto md_appendix_sz = 0L;
                if (compressed && (dq.bytesused < fr_payload_size))
                    md_appendix_sz = d4xx_md_size;
                else
                    md_appendix_sz = long(dq.bytesused) - fr_payload_size;

                if (md_appendix_sz >0 )
                {
                    md_start = buffer->get_frame_start() + dq.bytesused - md_appendix_sz;
                    md_size = (*(static_cast<uint8_t*>(md_start)));
                    int md_flags = (*(static_cast<uint8_t*>(md_start)+1));
                    // Use heuristics for metadata validation
                    if ((md_appendix_sz != md_size) || (!val_in_range(md_flags, {0x8e, 0x8f})))
                    {
                        md_size = 0;
                        md_start=nullptr;
                    }
                }
            }

            set_md_attributes(static_cast<uint8_t>(md_size),md_start);
        }

        bool buffers_mgr::verify_vd_md_sync() const
        {
            if ((buffers[e_video_buf]._file_desc > 0) && (buffers[e_metadata_buf]._file_desc > 0))
                if (buffers[e_video_buf]._dq_buf.sequence != buffers[e_metadata_buf]._dq_buf.sequence)
                {
                    LOG_ERROR("Non-sequential Video and Metadata v4l buffers");
                    return false;
                }
            return true;
        }

        // retrieve the USB specification attributed to a specific USB device.
        // This functionality is required to find the USB connection type for UVC device
        // Note that the input parameter is passed by value
        static usb_spec get_usb_connection_type(std::string path)
        {
            usb_spec res{usb_undefined};

            char usb_actual_path[PATH_MAX] = {0};
            if (realpath(path.c_str(), usb_actual_path) != nullptr)
            {
                path = std::string(usb_actual_path);
                std::string val;
                if(!(std::ifstream(path + "/version") >> val))
                    throw linux_backend_exception("Failed to read usb version specification");

                auto kvp = std::find_if(usb_spec_names.begin(),usb_spec_names.end(),
                     [&val](const std::pair<usb_spec, std::string>& kpv){ return (std::string::npos !=val.find(kpv.second));});
                        if (kvp != std::end(usb_spec_names))
                            res = kvp->first;
            }
            return res;
        }

        // Retrieve device video capabilities to discriminate video capturing and metadata nodes
        static uint32_t get_dev_capabilities(const std::string dev_name)
        {
            // RAII to handle exceptions
            std::unique_ptr<int, std::function<void(int*)> > fd(
                        new int (open(dev_name.c_str(), O_RDWR | O_NONBLOCK, 0)),
                        [](int* d){ if (d && (*d)) ::close(*d);});

            if(*fd < 0)
                throw linux_backend_exception(to_string() << __FUNCTION__ << ": Cannot open '" << dev_name);

            v4l2_capability cap = {};
            if(xioctl(*fd, VIDIOC_QUERYCAP, &cap) < 0)
            {
                if(errno == EINVAL)
                    throw linux_backend_exception(to_string() << __FUNCTION__ << " " << dev_name << " is no V4L2 device");
                else
                    throw linux_backend_exception(to_string() <<__FUNCTION__ << " xioctl(VIDIOC_QUERYCAP) failed");
            }

            return cap.device_caps;
        }

        void stream_ctl_on(int fd, v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE)
        {
            if(xioctl(fd, VIDIOC_STREAMON, &type) < 0)
                throw linux_backend_exception(to_string() << "xioctl(VIDIOC_STREAMON) failed for buf_type=" << type);
        }

        void stream_off(int fd, v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE)
        {
            if(xioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
                throw linux_backend_exception(to_string() << "xioctl(VIDIOC_STREAMOFF) failed for buf_type=" << type);
        }

        void req_io_buff(int fd, uint32_t count, std::string dev_name,
                        v4l2_memory mem_type, v4l2_buf_type type)
        {
            struct v4l2_requestbuffers req = { count, type, mem_type, {}};

            if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
            {
                if(errno == EINVAL)
                    LOG_ERROR(dev_name + " does not support memory mapping");
                else
                    throw linux_backend_exception("xioctl(VIDIOC_REQBUFS) failed");
            }
        }

        void v4l_uvc_device::foreach_uvc_device(
                std::function<void(const uvc_device_info&,
                                   const std::string&)> action)
        {
            // Enumerate all subdevices present on the system
            DIR * dir = opendir("/sys/class/video4linux");
            if(!dir)
            {
                LOG_INFO("Cannot access /sys/class/video4linux");
                return;
            }

            // Collect UVC nodes info to bundle metadata and video
            typedef std::pair<uvc_device_info,std::string> node_info;
            std::vector<node_info> uvc_nodes,uvc_devices;

            while (dirent * entry = readdir(dir))
            {
                std::string name = entry->d_name;
                if(name == "." || name == "..") continue;

                // Resolve a pathname to ignore virtual video devices
                std::string path = "/sys/class/video4linux/" + name;
                std::string real_path{};
                char buff[PATH_MAX] = {0};
                if (realpath(path.c_str(), buff) != nullptr)
                {
                    real_path = std::string(buff);
                    if (real_path.find("virtual") != std::string::npos)
                        continue;
                }

                try
                {
                    uint16_t vid, pid, mi;
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
                    auto valid_path = false;
                    for(auto i=0U; i < MAX_DEV_PARENT_DIR; ++i)
                    {
                        if(std::ifstream(path + "busnum") >> busnum)
                        {
                            if(std::ifstream(path + "devnum") >> devnum)
                            {
                                if(std::ifstream(path + "devpath") >> devpath)
                                {
                                    valid_path = true;
                                    break;
                                }
                            }
                        }
                        path += "../";
                    }
                    if(!valid_path)
                    {
#ifndef RS2_USE_CUDA
                       /* On the Jetson TX, the camera module is CSI & I2C and does not report as this code expects
                       Patch suggested by JetsonHacks: https://github.com/jetsonhacks/buildLibrealsense2TX */
                        LOG_INFO("Failed to read busnum/devnum. Device Path: " << path);
#endif
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

                    // Find the USB specification (USB2/3) type from the underlying device
                    // Use device mapping obtained in previous step to traverse node tree
                    // and extract the required descriptors
                    // Traverse from
                    // /sys/devices/pci0000:00/0000:00:xx.0/ABC/M-N/3-6:1.0/video4linux/video0
                    // to
                    // /sys/devices/pci0000:00/0000:00:xx.0/ABC/M-N/version
                    usb_spec usb_specification = get_usb_connection_type(real_path + "/../../../");

                    uvc_device_info info{};
                    info.pid = pid;
                    info.vid = vid;
                    info.mi = mi;
                    info.id = dev_name;
                    info.device_path = std::string(buff);
                    info.unique_id = busnum + "-" + devpath + "-" + devnum;
                    info.conn_spec = usb_specification;
                    info.uvc_capabilities = get_dev_capabilities(dev_name);

                    uvc_nodes.emplace_back(info, dev_name);
                }
                catch(const std::exception & e)
                {
                    LOG_INFO("Not a USB video device: " << e.what());
                }
            }
            closedir(dir);

            // Matching video and metadata nodes
            // UVC nodes shall be traversed in ascending order for metadata nodes assignment ("dev/video1, Video2..
            // Replace lexicographic with numeric sort to ensure "video2" is listed before "video11"
            std::sort(begin(uvc_nodes),end(uvc_nodes),[](const node_info& lhs, const node_info& rhs)
                        {
                            std::stringstream index_l(lhs.first.id.substr(lhs.first.id.find_first_of("0123456789")));
                            std::stringstream index_r(rhs.first.id.substr(rhs.first.id.find_first_of("0123456789")));
                            int left_id = 0;  index_l >> left_id;
                            int right_id = 0;  index_r >> right_id;
                            return left_id < right_id;
                        });

            // Assume for each metadata node with index N there is a origin streaming node with index (N-1)
            for (auto&& cur_node : uvc_nodes)
            {
                try
                {
                    if (!(cur_node.first.uvc_capabilities & V4L2_CAP_META_CAPTURE))
                        uvc_devices.emplace_back(cur_node);
                    else
                    {
                        if (uvc_devices.empty())
                        {
                            LOG_ERROR("uvc meta-node with no video streaming node encountered: " << std::string(cur_node.first));
                            continue;
                        }

                        // Update the preceding uvc item with metadata node info
                        auto uvc_node = uvc_devices.back();

                        if (uvc_node.first.uvc_capabilities & V4L2_CAP_META_CAPTURE)
                        {
                            LOG_ERROR("Consequtive uvc meta-nodes encountered: " << std::string(uvc_node.first) << " and " << std::string(cur_node.first) );
                            continue;
                        }

                        if (uvc_node.first.has_metadata_node)
                        {
                            LOG_ERROR( "Metadata node for uvc device: " << std::string(uvc_node.first) << " was previously assigned ");
                            continue;
                        }

                        // modify the last element with metadata node info
                        uvc_node.first.has_metadata_node = true;
                        uvc_node.first.metadata_node_id = cur_node.first.id;
                        uvc_devices.back() = uvc_node;
                    }
                }
                catch(const std::exception & e)
                {
                    LOG_ERROR("Failed to map meta-node "  << std::string(cur_node.first) <<", error encountered: " << e.what());
                }
            }

            try
            {
                // Dispatch registration for enumerated uvc devices
                for (auto&& dev : uvc_devices)
                    action(dev.first, dev.second);
            }
            catch(const std::exception & e)
            {
                LOG_ERROR("Registration of UVC device failed: " << e.what());
            }
        }

        v4l_uvc_device::v4l_uvc_device(const uvc_device_info& info, bool use_memory_map)
            : _name(""), _info(),
              _is_capturing(false),
              _is_alive(true),
              _is_started(false),
              _thread(nullptr),
              _named_mtx(nullptr),
              _use_memory_map(use_memory_map),
              _fd(-1),
              _stop_pipe_fd{}
        {
            foreach_uvc_device([&info, this](const uvc_device_info& i, const std::string& name)
            {
                if (i == info)
                {
                    _name = name;
                    _info = i;
                    _device_path = i.device_path;
                    _device_usb_spec = i.conn_spec;
                }
            });
            if (_name == "")
                throw linux_backend_exception("device is no longer connected!");

            _named_mtx = std::unique_ptr<named_mutex>(new named_mutex(_name, 5000));
        }

        v4l_uvc_device::~v4l_uvc_device()
        {
            _is_capturing = false;
            if (_thread && _thread->joinable()) _thread->join();
            for (auto&& fd : _fds)
            {
                try { if (fd) ::close(fd);} catch (...) {}
            }
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
                                                                  << "' is not natively supported by the running Linux kernel and likely requires a patch");
                                }
                            }
                        }
                    }
                    ++pixel_format.index;
                }

                set_format(profile);

                v4l2_streamparm parm = {};
                parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if(xioctl(_fd, VIDIOC_G_PARM, &parm) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_G_PARM) failed");

                parm.parm.capture.timeperframe.numerator = 1;
                parm.parm.capture.timeperframe.denominator = profile.fps;
                if(xioctl(_fd, VIDIOC_S_PARM, &parm) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_S_PARM) failed");

                // Init memory mapped IO
                negotiate_kernel_buffers(static_cast<size_t>(buffers));
                allocate_io_buffers(static_cast<size_t>(buffers));

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
                prepare_capture_buffers();

                // Synchronise stream requests for meta and video data.
                streamon();

                _is_capturing = true;
                _thread = std::unique_ptr<std::thread>(new std::thread([this](){ capture_loop(); }));
            }
        }

        void v4l_uvc_device::prepare_capture_buffers()
        {
            for (auto&& buf : _buffers) buf->prepare_for_streaming(_fd);
        }

        void v4l_uvc_device::stop_data_capture()
        {
            _is_capturing = false;
            _is_started = false;

            // Stop nn-demand frames polling
            signal_stop();

            _thread->join();
            _thread.reset();

            // Notify kernel
            streamoff();
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
                stop_data_capture();
            }

            if (_callback)
            {
                // Release allocated buffers
                allocate_io_buffers(0);

                // Release IO
                negotiate_kernel_buffers(0);

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
            char buff[1]={};
            if (write(_stop_pipe_fd[1], buff, 1) < 0)
            {
                 throw linux_backend_exception("Could not signal video capture thread to stop. Error write to pipe.");
            }
        }

        void v4l_uvc_device::poll()
        {
             fd_set fds{};
             FD_ZERO(&fds);
             for (auto fd : _fds)
             {
                 FD_SET(fd, &fds);
             }

            struct timespec mono_time;
            int ret = clock_gettime(CLOCK_MONOTONIC, &mono_time);
            if (ret) throw linux_backend_exception("could not query time!");

            struct timeval expiration_time = { mono_time.tv_sec + 5, mono_time.tv_nsec / 1000 };
            int val = 0;
            do {
                struct timeval remaining;
                ret = clock_gettime(CLOCK_MONOTONIC, &mono_time);
                if (ret) throw linux_backend_exception("could not query time!");

                struct timeval current_time = { mono_time.tv_sec, mono_time.tv_nsec / 1000 };
                timersub(&expiration_time, &current_time, &remaining);
                if (timercmp(&current_time, &expiration_time, <)) {
                    val = select(_max_fd + 1, &fds, nullptr, nullptr, &remaining);
                }
                else {
                    val = 0;
                }
            } while (val < 0 && errno == EINTR);

            if(val < 0)
            {
                stop_data_capture();
            }
            else
            {
                if(val > 0)
                {
                    if(FD_ISSET(_stop_pipe_fd[0], &fds) || FD_ISSET(_stop_pipe_fd[1], &fds))
                    {
                        if(!_is_capturing)
                        {
                            LOG_INFO("V4L stream is closed");
                            return;
                        }
                        else
                        {
                            LOG_ERROR("Stop pipe was signalled during streaming");
                            return;
                        }
                    }
                    else // Check and acquire data buffers from kernel
                    {
                        bool md_extracted = false;
                        buffers_mgr buf_mgr(_use_memory_map);
                        // RAII to handle exceptions
                        std::unique_ptr<int, std::function<void(int*)> > md_poller(new int(0),
                            [this,&buf_mgr,&md_extracted,&fds](int* d)
                            {
                                if (!md_extracted) acquire_metadata(buf_mgr,fds);
                                delete d;
                            });

                        if(FD_ISSET(_fd, &fds))
                        {
                            FD_CLR(_fd,&fds);
                            v4l2_buffer buf = {};
                            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                            buf.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
                            if(xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                            {
                                LOG_DEBUG_V4L("Dequeued empty buf for fd " << std::dec << _fd);
                                if(errno == EAGAIN)
                                    return;

                                throw linux_backend_exception(to_string() << "xioctl(VIDIOC_DQBUF) failed for fd: " << _fd);
                            }
                            LOG_DEBUG_V4L("Dequeued buf " << std::dec << buf.index << " for fd " << _fd << " seq " << buf.sequence);

                            auto buffer = _buffers[buf.index];
                            buf_mgr.handle_buffer(e_video_buf,_fd, buf,buffer);

                            if (_is_started)
                            {
                                if(buf.bytesused == 0)
                                {
                                    LOG_INFO("Empty video frame arrived");
                                    return;
                                }

                                // Relax the required frame size for compressed formats, i.e. MJPG, Z16H
                                // Drop partial and overflow frames (assumes D4XX metadata only)
                                bool compressed_format = val_in_range(_profile.format, { 0x4d4a5047U , 0x5a313648U});
                                bool partial_frame = (!compressed_format && (buf.bytesused < buffer->get_full_length() - MAX_META_DATA_SIZE));
                                bool overflow_frame = (buf.bytesused ==  buffer->get_length_frame_only() + MAX_META_DATA_SIZE);
                                if (partial_frame || overflow_frame)
                                {
                                    auto percentage = (100 * buf.bytesused) / buffer->get_full_length();
                                    std::stringstream s;
                                    if (partial_frame)
                                    {
                                        s << "Incomplete video frame detected!\nSize " << buf.bytesused
                                            << " out of " << buffer->get_full_length() << " bytes (" << percentage << "%)";
                                        if (overflow_frame)
                                        {
                                            s << ". Overflow detected: payload size " << buffer->get_length_frame_only();
                                            LOG_ERROR("Corrupted UVC frame data, underflow and overflow reported:\n" << s.str().c_str());
                                        }
                                    }
                                    else
                                    {
                                        if (overflow_frame)
                                            s << "overflow video frame detected!\nSize " << buf.bytesused
                                                << ", payload size " << buffer->get_length_frame_only();
                                    }
                                    librealsense::notification n = { RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED, 0, RS2_LOG_SEVERITY_WARN, s.str()};

                                    _error_handler(n);
                                }
                                else
                                {
                                    auto timestamp = (double)buf.timestamp.tv_sec*1000.f + (double)buf.timestamp.tv_usec/1000.f;
                                    timestamp = monotonic_to_realtime(timestamp);

                                    // Read metadata. For metadata note performs a blocking call to ensure video and metadata sync
                                    acquire_metadata(buf_mgr,fds,compressed_format);
                                    md_extracted = true;

                                    //if (val > 1)
                                    //    LOG_INFO("Frame buf ready, md size: " << std::dec << (int)buf_mgr.metadata_size() << " seq. id: " << buf.sequence);
                                    frame_object fo{ std::min(buf.bytesused - buf_mgr.metadata_size(), buffer->get_length_frame_only()), buf_mgr.metadata_size(),
                                        buffer->get_frame_start(), buf_mgr.metadata_start(), timestamp };

                                    buffer->attach_buffer(buf);
                                    buf_mgr.handle_buffer(e_video_buf,-1); // transfer new buffer request to the frame callback

                                    if (buf_mgr.verify_vd_md_sync())
                                    {
                                        //Invoke user callback and enqueue next frame
                                        _callback(_profile, fo, [buf_mgr]() mutable {
                                            buf_mgr.request_next_frame();
                                        });
                                    }
                                    else
                                    {
                                        LOG_WARNING("Video frame dropped, video and metadata buffers inconsistency");
                                    }
                                }
                            }
                            else
                            {
                                LOG_INFO("Video frame arrived in idle mode."); // TODO - verification
                            }
                        }
                        else
                        {
                            LOG_WARNING("FD_ISSET signal false - no data on video node sink");
                        }
                    }
                }
                else // (val==0)
                {
                    LOG_WARNING("Frames didn't arrived within 5 seconds");
                    librealsense::notification n = {RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT, 0, RS2_LOG_SEVERITY_WARN,  "Frames didn't arrived within 5 seconds"};

                    _error_handler(n);
                }
            }
        }

        void v4l_uvc_device::acquire_metadata(buffers_mgr & buf_mgr,fd_set &, bool compressed_format)
        {
            if (has_metadata())
                buf_mgr.set_md_from_video_node(compressed_format);
            else
                buf_mgr.set_md_attributes(0, nullptr);
        }

        void v4l_uvc_device::set_power_state(power_state state)
        {
            if (state == D0 && _state == D3)
            {
                map_device_descriptor();
            }
            if (state == D3 && _state == D0)
            {
                close(_profile);
                unmap_device_descriptor();
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
                if (errno == EIO || errno == EAGAIN || errno == EBUSY) // TODO: Log?
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

        bool v4l_uvc_device::has_metadata() const
        {
            return !_use_memory_map;
        }

        void v4l_uvc_device::streamon() const
        {
            stream_ctl_on(_fd);
        }

        void v4l_uvc_device::streamoff() const
        {
            stream_off(_fd);
        }

        void v4l_uvc_device::negotiate_kernel_buffers(size_t num) const
        {
            req_io_buff(_fd, num, _name,
                        _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR,
                        V4L2_BUF_TYPE_VIDEO_CAPTURE);
        }

        void v4l_uvc_device::allocate_io_buffers(size_t buffers)
        {
            if (buffers)
            {
                for(size_t i = 0; i < buffers; ++i)
                {
                    _buffers.push_back(std::make_shared<buffer>(_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, _use_memory_map, i));
                }
            }
            else
            {
                for(size_t i = 0; i < _buffers.size(); i++)
                {
                    _buffers[i]->detach_buffer();
                }
                _buffers.resize(0);
            }
        }

        void v4l_uvc_device::map_device_descriptor()
        {
            _fd = open(_name.c_str(), O_RDWR | O_NONBLOCK, 0);
            if(_fd < 0)
                throw linux_backend_exception(to_string() <<__FUNCTION__ << " Cannot open '" << _name);

            if (pipe(_stop_pipe_fd) < 0)
                throw linux_backend_exception(to_string() <<__FUNCTION__ << " Cannot create pipe!");

            if (_fds.size())
                throw linux_backend_exception(to_string() <<__FUNCTION__ << " Device descriptor is already allocated");

            _fds.insert(_fds.end(),{_fd,_stop_pipe_fd[0],_stop_pipe_fd[1]});
            _max_fd = *std::max_element(_fds.begin(),_fds.end());

            v4l2_capability cap = {};
            if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
            {
                if(errno == EINVAL)
                    throw linux_backend_exception(_name + " is not V4L2 device");
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

        void v4l_uvc_device::unmap_device_descriptor()
        {
            if(::close(_fd) < 0)
                throw linux_backend_exception("v4l_uvc_device: close(_fd) failed");

            if(::close(_stop_pipe_fd[0]) < 0)
               throw linux_backend_exception("v4l_uvc_device: close(_stop_pipe_fd[0]) failed");
            if(::close(_stop_pipe_fd[1]) < 0)
               throw linux_backend_exception("v4l_uvc_device: close(_stop_pipe_fd[1]) failed");

            _fd = 0;
            _stop_pipe_fd[0] = _stop_pipe_fd[1] = 0;
            _fds.clear();
        }

        void v4l_uvc_device::set_format(stream_profile profile)
        {
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
            else
                LOG_INFO("Video node was successfully configured to " << fourcc_to_string(fmt.fmt.pix.pixelformat) << " format" <<", fd " << std::dec << _fd);

            LOG_INFO("Trying to configure fourcc " << fourcc_to_string(fmt.fmt.pix.pixelformat));
        }

        v4l_uvc_meta_device::v4l_uvc_meta_device(const uvc_device_info& info, bool use_memory_map):
            v4l_uvc_device(info,use_memory_map),
            _md_fd(0),
            _md_name(info.metadata_node_id)
        {
        }

        v4l_uvc_meta_device::~v4l_uvc_meta_device()
        {
        }

        void v4l_uvc_meta_device::streamon() const
        {
            // Metadata stream shall be configured first to allow sync with video node
            stream_ctl_on(_md_fd,LOCAL_V4L2_BUF_TYPE_META_CAPTURE);

            // Invoke UVC streaming request
            v4l_uvc_device::streamon();

        }

        void v4l_uvc_meta_device::streamoff() const
        {
            v4l_uvc_device::streamoff();

            stream_off(_md_fd,LOCAL_V4L2_BUF_TYPE_META_CAPTURE);
        }

        void v4l_uvc_meta_device::negotiate_kernel_buffers(size_t num) const
        {
            v4l_uvc_device::negotiate_kernel_buffers(num);

            req_io_buff(_md_fd, num, _name,
                        _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR,
                        LOCAL_V4L2_BUF_TYPE_META_CAPTURE);
        }

        void v4l_uvc_meta_device::allocate_io_buffers(size_t buffers)
        {
            v4l_uvc_device::allocate_io_buffers(buffers);

            if (buffers)
            {
                for(size_t i = 0; i < buffers; ++i)
                {
                    _md_buffers.push_back(std::make_shared<buffer>(_md_fd, LOCAL_V4L2_BUF_TYPE_META_CAPTURE, _use_memory_map, i));
                }
            }
            else
            {
                for(size_t i = 0; i < _buffers.size(); i++)
                {
                    _md_buffers[i]->detach_buffer();
                }
                _md_buffers.resize(0);
            }
        }

        void v4l_uvc_meta_device::map_device_descriptor()
        {
            v4l_uvc_device::map_device_descriptor();

            if (_md_fd>0)
                throw linux_backend_exception(to_string() << _md_name << " descriptor is already opened");

            _md_fd = open(_md_name.c_str(), O_RDWR | O_NONBLOCK, 0);
            if(_md_fd < 0)
                throw linux_backend_exception(to_string() << "Cannot open '" << _md_name);

            //The minimal video/metadata nodes syncer will be implemented by using two blocking calls:
            // 1. Obtain video node data.
            // 2. Obtain metadata
            //     To revert to multiplexing mode uncomment the next line
            // _fds.push_back(_md_fd);
            _max_fd = *std::max_element(_fds.begin(),_fds.end());

            v4l2_capability cap = {};
            if(xioctl(_md_fd, VIDIOC_QUERYCAP, &cap) < 0)
            {
                if(errno == EINVAL)
                    throw linux_backend_exception(_md_name + " is no V4L2 device");
                else
                    throw linux_backend_exception(_md_name +  " xioctl(VIDIOC_QUERYCAP) for metadata failed");
            }

            if(!(cap.capabilities & V4L2_CAP_META_CAPTURE))
                throw linux_backend_exception(_md_name + " is not metadata capture device");

            if(!(cap.capabilities & V4L2_CAP_STREAMING))
                throw linux_backend_exception(_md_name + " does not support metadata streaming I/O");
        }


        void v4l_uvc_meta_device::unmap_device_descriptor()
        {
            v4l_uvc_device::unmap_device_descriptor();

            if(::close(_md_fd) < 0)
                throw linux_backend_exception("v4l_uvc_meta_device: close(_md_fd) failed");

            _md_fd = 0;
        }

        void v4l_uvc_meta_device::set_format(stream_profile profile)
        {
            // Select video node streaming format
            v4l_uvc_device::set_format(profile);

            // Configure metadata node stream format
            v4l2_format fmt{ };
            fmt.type = LOCAL_V4L2_BUF_TYPE_META_CAPTURE;

            if (xioctl(_md_fd, VIDIOC_G_FMT, &fmt))
                throw linux_backend_exception(_md_name + " ioctl(VIDIOC_G_FMT) for metadata node failed");

            if (fmt.type != LOCAL_V4L2_BUF_TYPE_META_CAPTURE)
                throw linux_backend_exception("ioctl(VIDIOC_G_FMT): " + _md_name + " node is not metadata capture");

            bool success = false;
            for (const uint32_t& request : { V4L2_META_FMT_D4XX, V4L2_META_FMT_UVC})
            {
                // Configure metadata format - try d4xx, then fallback to currently retrieve UVC default header of 12 bytes
                memcpy(fmt.fmt.raw_data,&request,sizeof(request));

                if(xioctl(_md_fd, VIDIOC_S_FMT, &fmt) >= 0)
                {
                    LOG_INFO("Metadata node was successfully configured to " << fourcc_to_string(request) << " format" <<", fd " << std::dec <<_md_fd);
                    success  =true;
                    break;
                }
                else
                {
                    LOG_WARNING("Metadata node configuration failed for " << fourcc_to_string(request));
                }
            }

            if (!success)
                throw linux_backend_exception(_md_name + " ioctl(VIDIOC_S_FMT) for metadata node failed");

        }

        void v4l_uvc_meta_device::prepare_capture_buffers()
        {
            // Meta node to be initialized first to enforce initial sync
            for (auto&& buf : _md_buffers) buf->prepare_for_streaming(_md_fd);

            // Request streaming for video node
            v4l_uvc_device::prepare_capture_buffers();
        }

        // Retrieve metadata from a dedicated UVC node. For kernels 4.16+
        void v4l_uvc_meta_device::acquire_metadata(buffers_mgr & buf_mgr,fd_set &fds, bool)
        {
            // Metadata is calculated once per frame
            if (buf_mgr.metadata_size())
                return;

            //Use blocking metadata node polling. Uncomment the next lines to revert to multiplexing I/O mode
            //if(FD_ISSET(_md_fd, &fds))
            {
                //FD_CLR(_md_fd,&fds);
                v4l2_buffer buf{};
                buf.type = LOCAL_V4L2_BUF_TYPE_META_CAPTURE;
                buf.memory = _use_memory_map ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;

                // W/O multiplexing this will create a blocking call for metadata node
                if(xioctl(_md_fd, VIDIOC_DQBUF, &buf) < 0)
                {
                    LOG_DEBUG_V4L("Dequeued empty buf for md fd " << std::dec << _md_fd);
                    return;

                    //throw linux_backend_exception(to_string() << "xioctl(VIDIOC_DQBUF) failed for metadata fd: " << _md_fd);
                }
                LOG_DEBUG_V4L("Dequeued md buf " << std::dec << buf.index << " for fd " << _md_fd << " seq " << buf.sequence);

                auto buffer = _md_buffers[buf.index];
                buf_mgr.handle_buffer(e_metadata_buf,_md_fd, buf,buffer);

                if (_is_started)
                {
                    static const size_t uvc_md_start_offset = sizeof(uvc_meta_buffer::ns) + sizeof(uvc_meta_buffer::sof);

                    if (buf.bytesused > uvc_md_start_offset )
                    {
                        // The first uvc_md_start_offset bytes of metadata buffer are generated by host driver
                        buf_mgr.set_md_attributes(buf.bytesused - uvc_md_start_offset,
                                                    buffer->get_frame_start() + uvc_md_start_offset);

                        buffer->attach_buffer(buf);
                        buf_mgr.handle_buffer(e_metadata_buf,-1); // transfer new buffer request to the frame callback
                    }
                    else
                    {
                        // Zero-size buffers generate empty md. Non-zero partial bufs handled as errors
                        if(buf.bytesused > 0)
                        {
                            std::stringstream s;
                            s << "Invalid metadata payload, size " << buf.bytesused;
                            LOG_WARNING(s.str());
                            _error_handler({ RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED, 0, RS2_LOG_SEVERITY_WARN, s.str()});
                        }
                    }
                }
                else
                {
                    LOG_INFO("Metadata frame arrived in idle mode.");
                }
            }
        }

        std::shared_ptr<uvc_device> v4l_backend::create_uvc_device(uvc_device_info info) const
        {
            auto v4l_uvc_dev = (!info.has_metadata_node) ? std::make_shared<v4l_uvc_device>(info) :
                                                           std::make_shared<v4l_uvc_meta_device>(info);

            return std::make_shared<platform::retry_controls_work_around>(v4l_uvc_dev);
        }

        std::vector<uvc_device_info> v4l_backend::query_uvc_devices() const
        {
            std::vector<uvc_device_info> uvc_nodes;
            v4l_uvc_device::foreach_uvc_device(
            [&uvc_nodes](const uvc_device_info& i, const std::string&)
            {
                uvc_nodes.push_back(i);
            });

            return uvc_nodes;
        }

        std::shared_ptr<command_transfer> v4l_backend::create_usb_device(usb_device_info info) const
        {
            auto dev = usb_enumerator::create_usb_device(info);
             if(dev)
                 return std::make_shared<platform::command_transfer_usb>(dev);
             return nullptr;
        }

        std::vector<usb_device_info> v4l_backend::query_usb_devices() const
        {
            auto device_infos = usb_enumerator::query_devices_info();
            // Give the device a chance to restart, if we don't catch
            // it, the watcher will find it later.
            if(tm_boot(device_infos)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                device_infos = usb_enumerator::query_devices_info();
            }
            return device_infos;
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
