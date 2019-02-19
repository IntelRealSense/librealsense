#include "android-hid.h"
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND
void librealsense::platform::android_hid_device::open(
        const std::vector<librealsense::platform::hid_profile> &iio_profiles) {

}

void librealsense::platform::android_hid_device::close() {

}

void librealsense::platform::android_hid_device::stop_capture() {

}

void librealsense::platform::android_hid_device::start_capture(
        librealsense::platform::hid_callback callback) {

}

std::vector<librealsense::platform::hid_sensor>
librealsense::platform::android_hid_device::get_sensors() {
    return std::vector<librealsense::platform::hid_sensor>();
}

std::vector<uint8_t> librealsense::platform::android_hid_device::get_custom_report_data(
        const std::string &custom_sensor_name, const std::string &report_name,
        librealsense::platform::custom_sensor_report_field report_field) {
    return std::vector<uint8_t>();
}

#endif