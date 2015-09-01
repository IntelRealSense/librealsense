#pragma once
#ifndef LIBREALSENSE_CAMERA_H
#define LIBREALSENSE_CAMERA_H

#include "types.h"

#include <mutex>

struct rs_camera
{
protected:
    class subdevice_handle;

    // Provides a stream of images to the user, and stores information about the current mode
    class stream_buffer
    {
        friend class subdevice_handle;

        struct frame
        {
            std::vector<uint8_t>                    pixels;
            int                                     number;

                                                    frame() : number() {}

            void                                    swap(frame & r) { pixels.swap(r.pixels); std::swap(number, r.number); }
        };

        rsimpl::stream_mode                         mode;
        frame                                       front, middle, back;
        std::mutex                                  mutex;
        volatile bool                               updated = false;
    public:
        const rsimpl::stream_mode &                 get_mode() const { return mode; }
        const void *                                get_image() const { return front.pixels.data(); }
        int                                         get_frame_number() const { return front.number; }

        void                                        set_mode(const rsimpl::stream_mode & mode);
        bool                                        update_image();
    };

    // Interfaces with a UVC subdevice, and unpacks the encoded frames into one or more stream_buffers
    class subdevice_handle
    {
        rsimpl::uvc::device_handle                  handle;
        rsimpl::subdevice_mode                      mode;
        std::vector<std::shared_ptr<stream_buffer>> streams;
    public:
                                                    subdevice_handle(rsimpl::uvc::device device, int subdevice_index);
                                                    ~subdevice_handle();

        rsimpl::uvc::device_handle *                get_handle() { return &handle; }
        void                                        set_mode(const rsimpl::subdevice_mode & mode, std::vector<std::shared_ptr<stream_buffer>> streams);
        void                                        start_streaming();
        void                                        stop_streaming();
    };

    uvc_context_t *                                 context;
    rsimpl::uvc::device                             device;
    const rsimpl::static_camera_info                camera_info;
    std::string                                     camera_name;

    rsimpl::stream_request                          requests[RS_STREAM_NUM];    // Indexed by RS_DEPTH, RS_COLOR, ...
    std::shared_ptr<stream_buffer>                  streams[RS_STREAM_NUM];     // Indexed by RS_DEPTH, RS_COLOR, ...
    std::vector<std::unique_ptr<subdevice_handle>>  subdevices;                 // Indexed by UVC subdevices number (0, 1, 2...)

    rsimpl::uvc::device_handle *                    first_handle = nullptr;
    rsimpl::calibration_info                        calib;
    bool                                            is_capturing;
  
public:
                                                    rs_camera(uvc_context_t * context, rsimpl::uvc::device device, const rsimpl::static_camera_info & camera_info);
                                                    ~rs_camera();

    const char *                                    get_name() const { return camera_name.c_str(); }

    void                                            enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                            enable_stream_preset(rs_stream stream, rs_preset preset);
    bool                                            is_stream_enabled(rs_stream stream) const { return (bool)streams[stream]; }
    void                                            configure_enabled_streams();
    void                                            start_capture();
    void                                            stop_capture();
    bool                                            is_streaming() { return is_capturing; }

    void                                            wait_all_streams();
    rs_format                                       get_image_format(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_mode().format; }
    const void *                                    get_image_pixels(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_image(); }
    int                                             get_image_frame_number(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_frame_number(); }
    float                                           get_depth_scale() const { return calib.depth_scale; }
    rs_intrinsics                                   get_stream_intrinsics(rs_stream stream) const;
    rs_extrinsics                                   get_stream_extrinsics(rs_stream from, rs_stream to) const;
    bool                                            supports_option(rs_option option) const { return camera_info.option_supported[option]; }

    virtual rsimpl::calibration_info                retrieve_calibration() = 0;
    virtual void                                    set_stream_intent() = 0;
    virtual void                                    set_option(rs_option option, int value) = 0;
    virtual int                                     get_option(rs_option option) = 0;
};

#endif
