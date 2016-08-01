// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_ZR300_H
#define LIBREALSENSE_ZR300_H

#include "motion-module.h"
#include "ds-device.h"


#define R200_PRODUCT_ID     0x0a80
#define LR200_PRODUCT_ID    0x0abf
#define ZR300_PRODUCT_ID    0x0acb
#define FISHEYE_PRODUCT_ID  0x0ad0

namespace rsimpl
{
    struct IMU_version
    {
        byte ver[4];
        byte size;
        byte CRC32[4];
    };
#pragma pack(push, 1)
    struct serial_number
    {
        IMU_version ver;
        byte MM_s_n[6];
        byte DS4_s_n[6];
        byte reserved[235];
    };

    struct fisheye_intrinsic 
    {
        IMU_version ver;
        float kf[9];
        float distf[5];
        byte reserved[191];
        
        operator rs_intrinsics () const
        {
            return{ 640, 480, kf[2], kf[4], kf[0], kf[4], RS_DISTORTION_FTHETA, { distf[0], 0, 0, 0, 0 } };
        }
    };


    struct mm_extrinsic
    {
        float rotation[9];
        float translation[3];

        operator rs_extrinsics () const {
            return{ {   rotation[0], rotation[1], rotation[2],
                        rotation[3], rotation[4], rotation[5],
                        rotation[6], rotation[7], rotation[8] },
                      { translation[0], translation[1], translation[2] } };
        }
    };
    struct IMU_extrinsic
    {
        IMU_version ver;
        mm_extrinsic  fe_to_imu;
        mm_extrinsic  fe_to_depth;
        mm_extrinsic  rgb_to_imu;
        mm_extrinsic  depth_to_imu;
    };

    struct MM_intrinsics
    {
        float bias_x;
        float bias_y;
        float bias_z;
        float scale_x;
        float scale_y;
        float scale_z;

        operator rs_motion_device_intrinsics() const{
            return{ { bias_x, bias_y, bias_z }, { scale_x, scale_y, scale_z } };
        };
    };

    struct IMU_intrinsic
    {
        IMU_version ver;
        MM_intrinsics acc_intrinsic;
        MM_intrinsics gyro_intrinsic;
        operator rs_motion_intrinsics() const{
            return{ rs_motion_device_intrinsics(acc_intrinsic), rs_motion_device_intrinsics(gyro_intrinsic) };
        };
    };


    struct motion_module_calibration
    {
        serial_number sn;
        fisheye_intrinsic fe_intrinsic;
        IMU_extrinsic mm_extrinsic;
        IMU_intrinsic imu_intrinsic;

    };
#pragma pop

    class zr300_camera final : public ds::ds_device
    {

        motion_module::motion_module_control    motion_module_ctrl;
        motion_module::mm_config                motion_module_configuration;

    protected:
        void toggle_motion_module_power(bool bOn);
        void toggle_motion_module_events(bool bOn);

    public:
        zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, motion_module_calibration fe_intrinsic);
        ~zr300_camera();

        void get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;
        void send_blob_to_device(rs_blob_type type, void * data, int size);

        void start_motion_tracking() override;
        void stop_motion_tracking() override;

        void start(rs_source source) override;
        void stop(rs_source source) override;

        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;

        rs_motion_intrinsics get_motion_intrinsics() const override;
        rs_extrinsics get_motion_extrinsics_from(rs_stream from) const override;
    private:
        motion_module_calibration fe_intrinsic;
    };
    motion_module_calibration read_fisheye_intrinsic(uvc::device & device);
    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device);
}

#endif
