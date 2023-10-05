// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <set>
#include <cstdint>


namespace librealsense {
namespace platform {


struct uvc_device_info;
struct hid_device_info;
struct usb_device_info;


// Helper functions for device list manipulation:
std::vector< uvc_device_info > filter_by_product( const std::vector< uvc_device_info > & devices,
                                                  const std::set< uint16_t > & pid_list );
std::vector< std::pair< std::vector< uvc_device_info >, std::vector< hid_device_info > > >
group_devices_and_hids_by_unique_id( const std::vector< std::vector< uvc_device_info > > & devices,
                                     const std::vector< hid_device_info > & hids );
std::vector< std::vector< uvc_device_info > >
group_devices_by_unique_id( const std::vector< platform::uvc_device_info > & devices );
void trim_device_list( std::vector< uvc_device_info > & devices, const std::vector< uvc_device_info > & chosen );
bool mi_present( const std::vector< uvc_device_info > & devices, uint32_t mi );
uvc_device_info get_mi( const std::vector< uvc_device_info > & devices, uint32_t mi );
std::vector< uvc_device_info > filter_by_mi( const std::vector< uvc_device_info > & devices, uint32_t mi );

std::vector< usb_device_info > filter_by_product( const std::vector< usb_device_info > & devices,
                                                  const std::set< uint16_t > & pid_list );
void trim_device_list( std::vector< usb_device_info > & devices, const std::vector< usb_device_info > & chosen );


}  // namespace platform
}  // namespace librealsense
