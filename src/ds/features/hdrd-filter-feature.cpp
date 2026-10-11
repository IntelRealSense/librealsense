// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.


#include <src/ds/features/hdrd-filter-feature.h>
#include <src/ds/d500/d500-device.h>
#include <src/ds/d500/hdrd-embedded-filter.h>
#include <src/ds/ds-private.h>
#include <src/uvc-sensor.h>

#include <librealsense2/h/rs_composite_option.h>
#include <librealsense2/h/rs_hdrd_control.h>


namespace librealsense {


/* static */ const feature_id hdrd_filter_feature::ID = "Improved Close Range filter feature";

hdrd_filter_feature::hdrd_filter_feature( d500_depth_sensor & depth_sensor )
{
    // Registers the ONE composite option this filter exposes, mirroring temporal_filter_feature.
    // ds::DS5_HKR_HDRD_CONTROL drives the same physical XU control formerly exposed as the
    // scalar "Improved Close Range Depth" option (close_range_xu_option, since removed).
    auto raw_depth_ep = std::dynamic_pointer_cast< uvc_sensor >( depth_sensor.get_raw_sensor() );
    depth_sensor.add_embedded_filter( std::make_shared< hdrd_embedded_filter >(
        raw_depth_ep,
        ds::DS5_HKR_HDRD_CONTROL,
        static_cast< uint32_t >( sizeof( rs2_hdrd_control ) ),
        RS2_COMPOSITE_OPTION_HDRD_CONTROL,
        "Improved Close Range Control (prototype) - use rs2_set/get_composite_option, see rs_hdrd_control.h" ) );
}

feature_id hdrd_filter_feature::get_id() const
{
    return ID;
}


}  // namespace librealsense
