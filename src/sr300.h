// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

#include "device.h"
#include "backend.h"


namespace rsimpl
{
    struct sr300;
    
    struct sr300_info : rs_device_info
    {
        rs_device* create(const rsimpl::uvc::backend& backend) const override;

        rs_device_info* clone() const override
        {
            return new sr300_info(*this);
        }

        sr300_info(uvc::uvc_device_info color,
                   uvc::uvc_device_info depth);

    private:
        uvc::uvc_device_info _color;
        uvc::uvc_device_info _depth;
    };

    std::vector<std::shared_ptr<rs_device_info>> pick_sr300_devices(std::vector<uvc::uvc_device_info>& uvc);

    struct sr300 : rs_device
    {
        sr300(const uvc::backend& backend,
              const uvc::uvc_device_info& color,
              const uvc::uvc_device_info& depth)
        {
            assign_endpoint(RS_SUBDEVICE_COLOR, std::make_shared<uvc_endpoint>(backend.create_uvc_device(color), this));
            assign_endpoint(RS_SUBDEVICE_DEPTH, std::make_shared<uvc_endpoint>(backend.create_uvc_device(depth), this));

            map_output(RS_FORMAT_Z16, RS_STREAM_DEPTH, "{5A564E49-2D90-4A58-920B-773F1F2C556B}");
        }

    private:

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
