// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ivcam-private.h"
#include "hw-monitor.h"

namespace rsimpl
{
    class sr300_camera;
    
    class sr300_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        std::shared_ptr<device_info> clone() const override
        {
            return std::make_shared<sr300_info>(*this);
        }

        sr300_info(uvc::uvc_device_info color,
                   uvc::uvc_device_info depth,
                   uvc::usb_device_info hwm);

    private:
        uvc::uvc_device_info _color;
        uvc::uvc_device_info _depth;
        uvc::usb_device_info _hwm;
    };

    std::vector<std::shared_ptr<device_info>> pick_sr300_devices(
        std::vector<uvc::uvc_device_info>& uvc, 
        std::vector<uvc::usb_device_info>& usb);

    class sr300_camera final : public device
    {
    public:
        sr300_camera(const uvc::backend& backend,
              const uvc::uvc_device_info& color,
              const uvc::uvc_device_info& depth,
              const uvc::usb_device_info& hwm_device)
            : _hw_monitor(backend.create_usb_device(hwm_device))
        {
            using namespace ivcam;

            auto fw_version = get_firmware_version_string();
            auto serial = get_module_serial_string();
            enable_timestamp(true, true);

            register_device("Intel RealSense SR300", fw_version, serial);

            // create uvc-endpoint from backend uvc-device
            auto color_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(color), this);
            auto depth_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(depth), this);
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera

            // map subdevice to endpoint
            assign_endpoint(RS_SUBDEVICE_COLOR, color_ep);
            assign_endpoint(RS_SUBDEVICE_DEPTH, depth_ep);

            // map formats, based on FW spec
            map_output(RS_FORMAT_Z16, RS_STREAM_DEPTH, "{5A564E49-2D90-4A58-920B-773F1F2C556B}");

            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_BACKLIGHT_COMPENSATION);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_BRIGHTNESS);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_CONTRAST);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_EXPOSURE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_GAIN);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_GAMMA);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_HUE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_SATURATION);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_SHARPNESS);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_WHITE_BALANCE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE);

            register_depth_xu(RS_OPTION_F200_LASER_POWER,          IVCAM_DEPTH_LASER_POWER);
            register_depth_xu(RS_OPTION_F200_ACCURACY,             IVCAM_DEPTH_ACCURACY);
            register_depth_xu(RS_OPTION_F200_MOTION_RANGE,         IVCAM_DEPTH_MOTION_RANGE);
            register_depth_xu(RS_OPTION_F200_CONFIDENCE_THRESHOLD, IVCAM_DEPTH_CONFIDENCE_THRESH);
            register_depth_xu(RS_OPTION_F200_FILTER_OPTION,        IVCAM_DEPTH_FILTER_OPTION);

            auto c = get_calibration();
            pose depth_to_color = { 
                transpose(reinterpret_cast<const float3x3 &>(c.Rt)), 
                          reinterpret_cast<const float3 &>(c.Tt) * 0.001f 
            }; 
            set_pose(RS_SUBDEVICE_DEPTH, inverse(depth_to_color));
            set_pose(RS_SUBDEVICE_COLOR, { { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } });
            set_depth_scale((c.Rmax / 0xFFFF) * 0.001f);
        }
    private:
        hw_monitor _hw_monitor;

        void register_depth_xu(rs_option opt, uint8_t id)
        {
            register_option(opt, RS_SUBDEVICE_DEPTH,
                std::make_shared<uvc_xu_option<uint8_t>>(
                    get_uvc_endpoint(RS_SUBDEVICE_DEPTH),
                    ivcam::depth_xu, id));
        }

        static rs_intrinsics make_depth_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        static rs_intrinsics make_color_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        float read_mems_temp() const;
        int read_ir_temp() const;

        void get_gvd(size_t sz, char * gvd, uint8_t gvd_cmd = ivcam::fw_cmd::GVD) const;
        std::string get_firmware_version_string(int gvd_cmd = static_cast<int>(ivcam::fw_cmd::GVD), int offset = 0) const;
        std::string get_module_serial_string() const;

        void force_hardware_reset() const;
        void enable_timestamp(bool colorEnable, bool depthEnable) const;
        void set_auto_range(int enableMvR, int16_t minMvR, int16_t maxMvR, 
            int16_t startMvR, int enableLaser, int16_t minLaser, 
            int16_t maxLaser, int16_t startLaser,
            int16_t ARUpperTH, int16_t ARLowerTH) const;

        ivcam::camera_calib_params get_calibration() const;
    };
}


//#ifndef LIBREALSENSE_SR300_H
//#define LIBREALSENSE_SR300_H
//
//#include <atomic>
//#include "ivcam-private.h"
//#include "ivcam-device.h"
//
//#define SR300_PRODUCT_ID 0x0aa5
//
//namespace rsimpl
//{
//
//    class sr300_camera final : public iv_camera
//    {
//        void set_fw_logger_option(double value);
//        unsigned get_fw_logger_option();
//
//    public:
//        sr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib);
//        ~sr300_camera() {};
//
//        void set_options(const rs_option options[], size_t count, const double values[]) override;
//        void get_options(const rs_option options[], size_t count, double values[]) override;
//
//        virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
//        virtual void stop_fw_logger() override;
//    };
//
//    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device);
//}
//
//#endif
