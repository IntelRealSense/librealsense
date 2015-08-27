#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include "camera.h"

namespace rsimpl
{
    namespace f200 { class IVCAMHardwareIO; }

    class f200_camera : public rs_camera
    {
        std::unique_ptr<f200::IVCAMHardwareIO> hardware_io;

    public:
        
        f200_camera(uvc_context_t * ctx, uvc_device_t * device);
        ~f200_camera();

        calibration_info retrieve_calibration() override final;
        void set_stream_intent() override final {}
        
        bool xu_read(uvc_device_handle_t * device, uint64_t xu_ctrl, void * buffer, uint32_t length);
        bool xu_write(uvc_device_handle_t * device, uint64_t xu_ctrl, void * buffer, uint32_t length);
        
        bool get_laser_power(uvc_device_handle_t * device, uint8_t & laser_power);
        bool set_laser_power(uvc_device_handle_t * device, uint8_t laser_power);             // 0 - 15
        
        bool get_accuracy(uvc_device_handle_t * device, uint8_t & accuracy);
        bool set_accuracy(uvc_device_handle_t * device, uint8_t accuracy);                   // 0 - 3
        
        bool get_motion_range(uvc_device_handle_t * device, uint8_t & motion_range);
        bool set_motion_range(uvc_device_handle_t * device, uint8_t motion_range);           // 0 - 100
        
        bool get_filter_option(uvc_device_handle_t * device, uint8_t & filter_option);
        bool set_filter_option(uvc_device_handle_t * device, uint8_t filter_option);         // 0 - 7
        
        bool get_confidence_threshhold(uvc_device_handle_t * device, uint8_t & conf_thresh);
        bool set_confidence_threshhold(uvc_device_handle_t * device, uint8_t conf_thresh);   // 0 - 15
        
        bool get_dynamic_fps(uvc_device_handle_t * device, uint8_t & dynamic_fps);
        bool set_dynamic_fps(uvc_device_handle_t * device, uint8_t dynamic_fps);             // One of: 2, 5, 15, 30, 60
        
        #define IVCAM_DEPTH_LASER_POWER         1
        #define IVCAM_DEPTH_ACCURACY            2
        #define IVCAM_DEPTH_MOTION_RANGE        3
        #define IVCAM_DEPTH_ERROR               4
        #define IVCAM_DEPTH_FILTER_OPTION       5
        #define IVCAM_DEPTH_CONFIDENCE_THRESH   6
        #define IVCAM_DEPTH_DYNAMIC_FPS         7

        #define IVCAM_COLOR_EXPOSURE_PRIORITY   1
        #define IVCAM_COLOR_AUTO_FLICKER        2
        #define IVCAM_COLOR_ERROR               3
        #define IVCAM_COLOR_EXPOSURE_GRANULAR   4  
    };
}

#endif
