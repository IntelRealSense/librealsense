#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
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
#pragma pack(push, 1)
struct z16y8_pixel { uint16_t z; uint8_t y; };
#pragma pack(pop)

class subdevice
{
    int fd;
    std::vector<buffer> buffers;
public:
    subdevice(const char * dev_name, bool force_format) : fd()
    {
        struct stat st;
        if(stat(dev_name, &st) < 0)
        {
            fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
            throw std::runtime_error("bad");
        }
        if(!S_ISCHR(st.st_mode))
        {
            fprintf(stderr, "%s is no device\n", dev_name);
            throw std::runtime_error("bad");
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
        if(fd < 0)
        {
            fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
            throw std::runtime_error("bad");
        }

        v4l2_capability cap = {};
        if(xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            if (EINVAL == errno)
            {
                fprintf(stderr, "%s is no V4L2 device\n", dev_name);
                throw std::runtime_error("bad");
            }
            else errno_exit("VIDIOC_QUERYCAP");
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            fprintf(stderr, "%s is no video capture device\n", dev_name);
            throw std::runtime_error("bad");
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
            throw std::runtime_error("bad");
        }

        // Select video input, video standard and tune here.
        v4l2_cropcap cropcap = {};
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
        {
            v4l2_crop crop = {};
            crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            crop.c = cropcap.defrect; // reset to default
            if (xioctl(fd, VIDIOC_S_CROP, &crop) < 0)
            {
                switch (errno)
                {
                case EINVAL: break; // Cropping not supported
                default: break; // Errors ignored
                }
            }
        } else {} // Errors ignored

        v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(force_format) // Force format?
        {
            fmt.fmt.pix.width       = 640;
            fmt.fmt.pix.height      = 480;
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
            fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
            if(xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) errno_exit("VIDIOC_S_FMT");
            // Note VIDIOC_S_FMT may change width and height
        }
        else
        {
            // Preserve original settings as set by v4l2-ctl for example
            if(xioctl(fd, VIDIOC_G_FMT, &fmt) < 0) errno_exit("VIDIOC_G_FMT");
        }

        // Buggy driver paranoia.
        unsigned int min = fmt.fmt.pix.width * 2;
        if(fmt.fmt.pix.bytesperline < min) fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if(fmt.fmt.pix.sizeimage < min) fmt.fmt.pix.sizeimage = min;

        // Init memory mapped IO
        v4l2_requestbuffers req = {};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
        {
            if(errno == EINVAL)
            {
                fprintf(stderr, "%s does not support memory mapping\n", dev_name);
                throw std::runtime_error("bad");
            }
            else errno_exit("VIDIOC_REQBUFS");
        }
        if(req.count < 2)
        {
            fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
            throw std::runtime_error("bad");
        }

        buffers.resize(req.count);
        for(int i=0; i<buffers.size(); ++i)
        {
            v4l2_buffer buf = {};
            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = i;
            if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) errno_exit("VIDIOC_QUERYBUF");

            buffers[i].length = buf.length;
            buffers[i].start = mmap(NULL /* start anywhere */,
                          buf.length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          fd, buf.m.offset);
            if(buffers[i].start == MAP_FAILED) errno_exit("mmap");
        }

        // Start capturing
        for(int i = 0; i < buffers.size(); ++i)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if(xioctl(fd, VIDIOC_QBUF, &buf) < 0) errno_exit("VIDIOC_QBUF");
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(fd, VIDIOC_STREAMON, &type) < 0) errno_exit("VIDIOC_STREAMON");
    }

    ~subdevice()
    {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(fd, VIDIOC_STREAMOFF, &type) < 0) errno_exit("VIDIOC_STREAMOFF");

        for(int i = 0; i < buffers.size(); ++i)
        {
            if(munmap(buffers[i].start, buffers[i].length) < 0) errno_exit("munmap");
        }

        if(close(fd) < 0) errno_exit("close");
    }

    template<class F> void poll(F f)
    {
        for (;;)
        {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r) {
                    if (EINTR == errno)
                            continue;
                    errno_exit("select");
            }

            if (0 == r) {
                    fprintf(stderr, "select timeout\n");
                    exit(EXIT_FAILURE);
            }

            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if(xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
            {
                if(errno == EAGAIN) continue;
                errno_exit("VIDIOC_DQBUF");
            }
            assert(buf.index < buffers.size());

            f(buffers[buf.index].start, buf.bytesused);

            if(xioctl(fd, VIDIOC_QBUF, &buf) < 0) errno_exit("VIDIOC_QBUF");
            break;
        }
    }
};

int main(int argc, char * argv[])
{
    subdevice dev0("/dev/video0", true);
    subdevice dev1("/dev/video1", false);

    // Open a GLFW window
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(960, 480, "V4L2 test", 0, 0);
    glfwMakeContextCurrent(win);

    // While window is open
    uint16_t z[320*240]; uint8_t y[320*240];
    uint8_t yuy2[640*480*2];
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        dev0.poll([&](const void * data, size_t size)
        {
            memcpy(yuy2, data, size);
            printf("%d %d\n", size, 640*480*2);
        });

        dev1.poll([&](const void * data, size_t size)
        {
            auto in = reinterpret_cast<const z16y8_pixel *>(data);
            for(int i=0; i<320*240; ++i)
            {
                z[i] = in[i].z;
                y[i] = in[i].y;
            }
            printf("%d %d\n", size, 320*240*3);
        });

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
        glDrawPixels(320, 240, GL_LUMINANCE, GL_UNSIGNED_SHORT, z);
        glRasterPos2i(640, 240);
        glDrawPixels(320, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, y);

        glPopMatrix();
        glfwSwapBuffers(win);
    }
    glfwDestroyWindow(win);
    glfwTerminate();

    return EXIT_SUCCESS;
}

