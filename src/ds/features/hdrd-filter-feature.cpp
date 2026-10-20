// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.


#include <src/ds/features/hdrd-filter-feature.h>
#include <src/ds/d500/d500-device.h>
#include <src/ds/d500/hdrd-embedded-filter.h>
#include <src/ds/ds-private.h>
#include <src/ds/composite-xu-option.h>
#include <src/uvc-sensor.h>

#include <librealsense2/h/rs_composite_option.h>
#include <librealsense2/h/rs_hdrd_control.h>


namespace librealsense {


// D500_CAMERA_CID_MINZ range descriptor; step is 1 per slot (all-integer params). Defaults
// track LibRS's rs2_hdrd_control field docs.
// GMSL cannot query ranges (V4L2 array controls report one scalar bound and the FW
// registers hold only the current value), so these tables are the only source of the MIPI
// range. USB reads it from FW and ignores this desc.
static constexpr int32_t minz_min[]  = { 0, 0, 1, 0,   0, 0, 0 };
static constexpr int32_t minz_max[]  = { 1, 1, 2, 2, 256, 2, 65535 };
static constexpr int32_t minz_step[] = { 1, 1, 1, 1,   1, 1, 1 };
static constexpr int32_t minz_def[]  = { 0, 0, 1, 0, 126, 1, 0 };

static constexpr dpp_control_desc minz_desc = {
    0x0008,  // ctl_id (D500_DPP_XU_MINZ_CONTROL_ID = BIT(3))
    7,       // param_count
    0x00,    // param_type (all-integer)
    minz_min, minz_max, minz_step, minz_def
};


/* static */ const feature_id hdrd_filter_feature::ID = "Improved Close Range filter feature";

hdrd_filter_feature::hdrd_filter_feature( d500_depth_sensor & depth_sensor, bool is_mipi )
{
    // Registers the ONE composite option this filter exposes, mirroring temporal_filter_feature.
    // ds::DS5_HKR_HDRD_CONTROL drives the same physical XU control formerly exposed as the
    // scalar "Improved Close Range Depth" option (close_range_xu_option, since removed).
    auto raw_depth_ep = std::dynamic_pointer_cast< uvc_sensor >( depth_sensor.get_raw_sensor() );
    auto option = composite_xu_option::create(
        is_mipi,
        raw_depth_ep,
        ds::depth_xu,
        ds::DS5_HKR_HDRD_CONTROL,
        static_cast< uint32_t >( sizeof( rs2_hdrd_control ) ),
        "Improved Close Range Control (prototype) - use rs2_set/get_composite_option, see rs_hdrd_control.h",
        minz_desc );
    depth_sensor.add_embedded_filter( std::make_shared< hdrd_embedded_filter >(
        std::move( option ), RS2_COMPOSITE_OPTION_HDRD_CONTROL ) );
}

feature_id hdrd_filter_feature::get_id() const
{
    return ID;
}


}  // namespace librealsense
