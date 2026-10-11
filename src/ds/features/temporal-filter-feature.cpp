// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.


#include <src/ds/features/temporal-filter-feature.h>
#include <src/ds/d500/d500-device.h>
#include <src/ds/d500/composite-embedded-filter.h>
#include <src/ds/ds-private.h>
#include <src/proc/temporal-embedded-filter.h>
#include <src/uvc-sensor.h>

#include <librealsense2/h/rs_composite_option.h>
#include <librealsense2/h/rs_temporal_filter_dpp.h>


namespace librealsense {


/* static */ const feature_id temporal_filter_feature::ID = "Temporal filter feature";

temporal_filter_feature::temporal_filter_feature( d500_depth_sensor & depth_sensor )
{
    // Registers the ONE composite option this filter exposes. No dedicated alias type:
    // temporal_embedded_filter is the RS2_EXTENSION_* identity, and this is the only place that
    // ever constructs it (compare hdrd-embedded-filter.h, which needs a named alias).
    auto raw_depth_ep = std::dynamic_pointer_cast< uvc_sensor >( depth_sensor.get_raw_sensor() );
    depth_sensor.add_embedded_filter( std::make_shared<
        composite_embedded_filter< temporal_embedded_filter, RS2_EMBEDDED_FILTER_TYPE_TEMPORAL > >(
        raw_depth_ep,
        ds::DS5_HKR_TEMPORAL_FILTER_DPP,
        static_cast< uint32_t >( sizeof( rs2_temporal_filter_dpp_config ) ),
        RS2_COMPOSITE_OPTION_TEMPORAL_FILTER_DPP,
        "Temporal Filter DPP (prototype) - use rs2_set/get_composite_option, see rs_temporal_filter_dpp.h" ) );
}

feature_id temporal_filter_feature::get_id() const
{
    return ID;
}


}  // namespace librealsense
