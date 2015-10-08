#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <functional>

#include <GLFW/glfw3.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>

static int xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (r < 0 && errno == EINTR);
    return r;
}

struct buffer { void * start; size_t length; };
#pragma pack(push, 1)
struct z16y8_pixel { uint16_t z; uint8_t y; };
#pragma pack(pop)

void throw_error(const char * s)
{
    std::ostringstream ss;
    ss << s << " error " << errno << ", " << strerror(errno);
    throw std::runtime_error(s);
}

void warn_error(const char * s)
{
    std::cerr << s << " error " << errno << ", " << strerror(errno) << std::endl;
}

class subdevice
{
    std::string dev_name;
    int vid, pid;
    int fd;
    std::vector<buffer> buffers;
    std::function<void(const void *, size_t)> callback;
public:
    subdevice(const std::string & name) : dev_name("/dev/" + name), fd()
    {
        struct stat st;
        if(stat(dev_name.c_str(), &st) < 0)
        {
            std::ostringstream ss; ss << "Cannot identify '" << dev_name << "': " << errno << ", " << strerror(errno);
            throw std::runtime_error(ss.str());
        }
        if(!S_ISCHR(st.st_mode)) throw std::runtime_error(dev_name + " is no device");

        std::string path = "/sys/class/video4linux/" + name + "/device/input";
        DIR * dir = opendir(path.c_str());
        if(!dir) throw std::runtime_error("Could not access " + path);
        while(dirent * entry = readdir(dir))
        {
            std::string inputName = entry->d_name;
            if(inputName.size() < 5 || inputName.substr(0,5) != "input") continue;
            if(!(std::ifstream(path + "/" + inputName + "/id/vendor") >> std::hex >> vid)) throw std::runtime_error("Failed to read vendor ID");
            if(!(std::ifstream(path + "/" + inputName + "/id/product") >> std::hex >> pid)) throw std::runtime_error("Failed to read product ID");
        }
        closedir(dir);

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
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(fd, VIDIOC_STREAMOFF, &type) < 0) warn_error("VIDIOC_STREAMOFF");

        for(int i = 0; i < buffers.size(); ++i)
        {
            if(munmap(buffers[i].start, buffers[i].length) < 0) warn_error("munmap");
        }

        if(close(fd) < 0) warn_error("close");
    }

    int get_vid() const { return vid; }
    int get_pid() const { return pid; }

    void get_control(int control, void * data, size_t size)
    {
        struct uvc_xu_control_query q = {2, control, UVC_GET_CUR, size, reinterpret_cast<uint8_t *>(data)};
        if(xioctl(fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_GET_CUR");
    }

    void set_control(int control, void * data, size_t size)
    {
        struct uvc_xu_control_query q = {2, control, UVC_SET_CUR, size, reinterpret_cast<uint8_t *>(data)};
        if(xioctl(fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_SET_CUR");
    }

    void start_capture(int width, int height, int fourcc, std::function<void(const void * data, size_t size)> callback)
    {
        v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = width;
        fmt.fmt.pix.height      = height;
        fmt.fmt.pix.pixelformat = fourcc;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        if(xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) throw_error("VIDIOC_S_FMT");
        // Note VIDIOC_S_FMT may change width and height

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
        for(int i=0; i<buffers.size(); ++i)
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
        for(int i = 0; i < buffers.size(); ++i)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if(xioctl(fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(fd, VIDIOC_STREAMON, &type) < 0) throw_error("VIDIOC_STREAMON");

        this->callback = callback;
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

        struct timeval tv = {};
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
                assert(buf.index < sub->buffers.size());

                sub->callback(sub->buffers[buf.index].start, buf.bytesused);

                if(xioctl(sub->fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
            }
        }
    }
};

int main(int argc, char * argv[])
{
    std::vector<std::unique_ptr<subdevice>> subdevices;

    DIR * dir = opendir("/sys/class/video4linux");
    if(!dir) throw std::runtime_error("Cannot access /sys/class/video4linux");
    while (dirent * entry = readdir(dir))
    {
        std::string name = entry->d_name;
        if(name == "." || name == "..") continue;
        std::unique_ptr<subdevice> sub(new subdevice(name));
        subdevices.push_back(move(sub));
    }
    closedir(dir);

    uint16_t z[640*480] = {};
    uint8_t yuy2[640*480*2] = {};
    std::vector<subdevice *> devs;
    if(subdevices.size() >= 2 && subdevices[0]->get_vid() == 0x8086 && subdevices[0]->get_pid() == 0xa66)
    {
        std::cout << "F200 detected!" << std::endl;
        subdevices[0]->start_capture(640, 480, V4L2_PIX_FMT_YUYV, [&](const void * data, size_t size) { if(size == sizeof(yuy2)) memcpy(yuy2, data, size); });
        subdevices[1]->start_capture(640, 480, v4l2_fourcc('I','N','V','R'), [&](const void * data, size_t size) { if(size == sizeof(z)) memcpy(z, data, size); });
        devs = {subdevices[0].get(), subdevices[1].get()};
    }
    else if(subdevices.size() >= 3 && subdevices[0]->get_vid() == 0x8086 && subdevices[0]->get_pid() == 0xa80)
    {
        std::cout << "R200 detected!" << std::endl;
        subdevices[2]->start_capture(640, 480, V4L2_PIX_FMT_YUYV, [&](const void * data, size_t size) { if(size == sizeof(yuy2)) memcpy(yuy2, data, size); });
        devs = {subdevices[2].get()};
    }
    else if(!subdevices.empty())
    {
        std::cout << "Unknown V4L2 device detected, vid=0x" << std::hex << subdevices[0]->get_vid() << ", pid=0x" << subdevices[0]->get_pid() << std::endl;
    }

    // Open a GLFW window
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 480, "V4L2 test", 0, 0);
    glfwMakeContextCurrent(win);

    // While window is open
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        subdevice::poll(devs);

        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);

        glRasterPos2i(0, 480);
        glDrawPixels(640, 480, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, yuy2);
        glRasterPos2i(640, 480);
        glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_SHORT, z);

        glPopMatrix();
        glfwSwapBuffers(win);
    }
    glfwDestroyWindow(win);
    glfwTerminate();

    return EXIT_SUCCESS;
}

// struct uvc_xu_control_query q;
// q.unit = 2;
// q.selector = cmd;
// q.query = UVC_GET_CUR/UVC_SET_CUR;
// q.size = sizeof(x);
// q.data = &x;
// ioctl(fd, UVCIOC_CTRL_QUERY, &q)
