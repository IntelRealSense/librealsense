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
    struct version
    {
        uint32_t ver;
        byte Size;
        uint32_t CRC32;
    };

    struct serial_number
    {
        version ver;
        byte Size;
        uint32_t CRC32;
        byte MM_s_n[6];
        byte DS4_s_n[6];
        byte reserved[235];
    };

    struct fe_intrinsic 
    {
        version ver;
        float3x3 kf;
        float distf[5];
        byte reserved[191];
        operator rs_intrinsics () const { return{ 640, 480, kf.x.z, kf.y.z, kf.x.x, kf.y.y, RS_DISTORTION_FTHETA, { 0, 0, 0, 0, 0 } }; }
    };

    struct mm_extrinsic
    {
        version ver;
        float3x3 rotation_fe_to_imu;
        float3 translation_fe_to_imu;
        float3x3 rotation_fe_to_depth;
        float3 translation_fe_to_depth;
        float3x3 rotation_rgb_to_imu;
        float3 translation_rgb_to_imu;
        float3x3 rotation_depth_to_imu;
        float3 translation_depth_to_imu;
    };

    struct mm_intrinsic
    {
        float Bias_x;
        float Bias_y;
        float Bias_z;
        float scale_x;
        float scale_y;
        float scale_z;
    };

    struct IMU_intrinsic
    {
        version ver;
        mm_intrinsic acc_intrinsic;
        mm_intrinsic gyro_intrinsic;
    };

    struct fisheye_intrinsic
    {
        serial_number sn;
        fe_intrinsic fe_intrinsic;
        mm_extrinsic mm_extrinsic;
        IMU_intrinsic imu_intrinsic;

    };

    class zr300_camera final : public ds::ds_device
    {

        motion_module::motion_module_control    motion_module_ctrl;
        motion_module::mm_config                motion_module_configuration;

    protected:
        void toggle_motion_module_power(bool bOn);
        void toggle_motion_module_events(bool bOn);

    public:
        zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, fisheye_intrinsic fe_intrinsic);
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

    private:
        fisheye_intrinsic fe_intrinsic;
    };
    fisheye_intrinsic read_fisheye_intrinsic(uvc::device & device);
    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device);
}

#endif
