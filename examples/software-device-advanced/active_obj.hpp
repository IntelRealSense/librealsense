/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_ACTIVE_OBJ_HPP
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_ACTIVE_OBJ_HPP


#include <librealsense2/rs.hpp> // Librealsense Cross-Platform API
#include <librealsense2/hpp/rs_internal.hpp> // Librealsense Software Device header

#include <librealsense/rs.hpp> // Legacy Librealsense for wrapping

#include <atomic>
#include <map>
#include <set>

class legacy_active_obj : public std::enable_shared_from_this<legacy_active_obj> {
private:
    std::atomic<bool> alive; // In charge of signaling destruction from rs2::software_device
    rs::device * legacy_dev; // Legacy realsense device handle
    std::thread heart; // Thread in charge of polling loop

    // wrapper's sensors. It is safe to hold these as destroying the rs2::software_device
    // causes the legacy_active_obj to be destroyed, freeing these references and allowing the sensors
    // to also deconstruct.
    std::map<rs2_stream, rs2::software_sensor> sensors;
    // Current profiles we have opened. Used for polling changes to open streams
    std::map<int, rs2::video_stream_profile> active_profiles;

    // Caches current values of all the supported options.  Used to poll for changes
    // made to the options by the user of the rs2::software_device
    std::map<rs::option, float> cur_options;

    void on_frame(rs::frame f);
    void heartbeat();

    void map_infos(rs2::software_device dev);
    void map_profiles(rs2::software_device dev);
    void map_options();

public:
    legacy_active_obj(rs::context& legacy_ctx, int legacy_dev_idx, rs2::software_device dev);
    ~legacy_active_obj();

    // needs to be a separate function so that enable_shared is properly bound
    void finalize_binding(rs2::software_device dev);

};

#endif