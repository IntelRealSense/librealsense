// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "stream.h"
#include <src/platform/stream-profile.h>

#include <memory>
#include <vector>


namespace librealsense
{
    // Supports two color streams over USB endpoints (pins) of the depth interface, instead of through a dedicated sensor
    class d500_dual_color : public virtual d500_device
    {
    public:
        d500_dual_color( std::shared_ptr< const d500_info > const & dev_info );


    protected:
        std::shared_ptr< stream_interface > _color_stream_1;
        std::shared_ptr< stream_interface > _color_stream_2;
        // Raw endpoint tied specifically to the pin whose fd hosts the RGB Processing Unit.
        // Only populated when discovery finds a distinct RGB pin (e.g. Linux V4L2, where each
        // /dev/videoN's fd exposes only its own PU chain's CIDs). Left null on backends where
        // any pin resolves to the same source (e.g. WMF), in which case the RGB controls
        // register against the aggregate depth raw endpoint and rely on node-based routing.
        std::shared_ptr< uvc_sensor > _raw_rgb_ep;

    private:
        // Stream-combination rules for the shared imagers, registered as validators at construction.
        void close_range_allowed_or_throw( const stream_profiles & requests ) const;
        void frame_rates_allowed_or_throw( const stream_profiles & requests ) const;

        void register_color_extrinsics();
        void register_color_metadata();
        void register_ae_policy_option();
        void register_color_options( std::shared_ptr< const d500_info > const & dev_info );
        std::shared_ptr< uvc_sensor > pick_rgb_pu_raw_endpoint(
            std::shared_ptr< const d500_info > const & dev_info,
            const platform::processing_unit & rgb_pu );

        // Stream-id resolver: route color pins (NV12/M420/YUY2) to Color 1 / Color 2 streams
        static void resolve_color_stream( const std::vector< platform::stream_profile > & all,
                                          const platform::stream_profile & p, rs2_stream & type, int & index );
        static bool is_color_pin( const std::vector< platform::stream_profile > & all, uint32_t pin );
    };
}
