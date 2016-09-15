// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5_PRIVATE_H
#define LIBREALSENSE_DS5_PRIVATE_H

#include "uvc.h"
#include <mutex>

namespace rsimpl {
    namespace ds5 {

    const int gvd_size = 1024;

    enum gvd_offset_fields : int32_t // data were taken from CommandsDS5.xml
    {
        fw_version_offset = 12,
        asic_module_serial_offset = 115
    };

    enum calibration_modules_id
    {
        depth_module_id,
        left_imager_id,
        right_imager_id,
        rgb_imager_id,
        fisheye_imager_id,
        imu_id,
        lens_module_id,
        projector_module_id,
        max_calib_module_id
    };

    enum calibration_table_id
    {
        coefficients_table_id   =   25,
        depth_calibration_id    =   31,
        rgb_calibration_id      =   32,
        fisheye_calibration_id  =   33,
        imu_calibration_id      =   34,
        lens_shading_id         =   35,
        projector_id            =   36
    };

    enum ds5_rect_resolutions : unsigned short
    {
        res_1920_1080,
        res_1280_720,
        res_960_540,
        res_640_480,
        res_854_480,
        res_640_360,
        res_432_240,
        res_320_240,
        res_480_270,
        reserved_1,
        reserved_2,
        reserved_3,
        max_ds5_rect_resoluitons
    };

    static std::map< ds5_rect_resolutions, int2> resolutions_list = {
        { res_320_240,  { 320, 240 } },
        { res_432_240,  { 432, 240 } },
        { res_480_270,  { 480, 270 } },
        { res_640_360,  { 640, 360 } },
        { res_640_480,  { 640, 480 } },
        { res_854_480,  { 854, 480 } },
        { res_960_540,  { 960, 540 } },
        { res_1280_720, { 1280, 720 } },
        { res_1920_1080,{ 1920, 1080 } },
    };

    struct ds5_calibration
    {
        uint16_t        version;                        // major.minor
        rs_intrinsics   left_imager_intrinsic;
        rs_intrinsics   right_imager_intrinsic;
        rs_intrinsics   depth_intrinsic[max_ds5_rect_resoluitons];
        rs_extrinsics   left_imager_extrinsic;
        rs_extrinsics   right_imager_extrinsic;
        rs_extrinsics   depth_extrinsic;
        std::map<calibration_table_id, bool> data_present;

        ds5_calibration(): version(0)
        {
            data_present.emplace(coefficients_table_id, false);
            data_present.emplace(depth_calibration_id, false);
            data_present.emplace(rgb_calibration_id, false);
            data_present.emplace(fisheye_calibration_id, false);
            data_present.emplace(imu_calibration_id, false);
            data_present.emplace(lens_shading_id, false);
            data_present.emplace(projector_id, false);
        };
    };

    std::string read_firmware_version(uvc::device & device);

    // Claim USB interface used for device
    void claim_ds5_monitor_interface(uvc::device & device);
    void claim_ds5_motion_module_interface(uvc::device & device);

    // Read device state
    void get_gvd_raw(uvc::device & device, std::timed_mutex & mutex, size_t sz, unsigned char * gvd);
    void get_string_of_gvd_field(uvc::device & device, std::timed_mutex & mutex, std::string & str, gvd_offset_fields offset);
    void read_calibration(uvc::device & dev, std::timed_mutex & mutex, ds5_calibration& calib);

    // XU read/write
    void get_laser_power(const uvc::device & device, uint8_t & laser_power);
    void set_laser_power(uvc::device & device, uint8_t laser_power);
    void set_lr_exposure(uvc::device & device, uint16_t exposure);
    uint16_t get_lr_exposure(const uvc::device & device);


} //namespace rsimpl::ds5
} // namespace rsimpl

#endif  // DS5_PRIVATE_H
