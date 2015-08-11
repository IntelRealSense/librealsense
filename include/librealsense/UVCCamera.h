#pragma once

#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "../../../src/rs-internal.h" // TODO: Migrate this header to be internal to LibRealsense
#include "libuvc/libuvc.h"

namespace rs
{

////////////////
// UVC Camera //
////////////////

class UVCCamera : public rs_camera
{
    NO_MOVE(UVCCamera);
 
protected:
    
    struct StreamInterface
    {
        UVCCamera * camera = nullptr;
        uvc_device_handle_t * uvcHandle = nullptr;
        uvc_frame_format fmt = UVC_FRAME_FORMAT_UNKNOWN;
		uvc_stream_ctrl_t ctrl = uvc_stream_ctrl_t{}; // {0};
    };
        
    uvc_device_t * hardware = nullptr;   
    bool hardwareInitialized = false;
    std::map<int, StreamInterface *> streamInterfaces;
    
    static void cb(uvc_frame_t * frame, void * ptr)
    {
        StreamInterface * stream = static_cast<StreamInterface*>(ptr);
        stream->camera->frameCallback(frame, stream);
    }
    
    void frameCallback(uvc_frame_t * frame, StreamInterface * stream);

    bool OpenStreamOnSubdevice(uvc_device_t * dev, uvc_device_handle_t *& h, int idx);
    
public:
    
    UVCCamera(uvc_device_t * device, int num);
    ~UVCCamera();
};

} // end namespace rs

#endif