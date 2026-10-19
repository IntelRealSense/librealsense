// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include "composite-mipi-xu-option.h"

#include <src/librealsense-exception.h>
#include <rsutils/string/from.h>

#include <cstring>

namespace librealsense {

composite_mipi_xu_option::composite_mipi_xu_option( std::weak_ptr< uvc_sensor > ep,
                                                    platform::extension_unit xu,
                                                    uint8_t ctrl_id,
                                                    uint32_t wire_size,
                                                    std::string description,
                                                    const dpp_control_desc & desc )
    : composite_usb_xu_option( std::move( ep ), xu, ctrl_id, wire_size, std::move( description ) )
    , _desc( desc )
{
    // Validate the desc fits in the wire once, so the memcpy sites below can trust the invariant.
    const size_t required = sizeof( dpp_header ) + _desc.param_count * sizeof( int32_t );
    if( required > wire_size )
        throw invalid_value_exception( rsutils::string::from()
                                        << "composite_mipi_xu_option: dpp_control_desc (ctl_id=" << (int)_desc.ctl_id
                                        << ") needs " << required << " bytes but wire_size is only " << wire_size );
}

// Ranges are NOT available from the driver or FW over GMSL: V4L2's VIDIOC_QUERY_EXT_CTRL only
// reports one aggregate scalar bound for an array control, and the FW register map exposes just
// the current value of each control. Unlike USB (UVC GET_MIN/MAX/DEF/RES answered by FW), the
// SDK therefore keeps static per-slot tables in `_desc` and builds each bound as a full struct
// (header + active params + reserved zero-fill), matching composite_xu_option::get_raw_range.
platform::control_range composite_mipi_xu_option::read_range_from_device( platform::uvc_device & /*dev*/ ) const
{
    auto build = [this]( const int32_t * slots )
    {
        std::vector< uint8_t > v( wire_size(), 0 );
        dpp_header hdr{};
        hdr.version = DPP_HEADER_CURRENT_VERSION;
        hdr.flags = 0;
        hdr.ctl_id = _desc.ctl_id;
        hdr.param_count = _desc.param_count;
        hdr.param_type = _desc.param_type;
        std::memcpy( v.data(), &hdr, sizeof( dpp_header ) );
        std::memcpy( v.data() + sizeof( dpp_header ), slots, _desc.param_count * sizeof( int32_t ) );
        return v;
    };
    return platform::control_range( build( _desc.min ), build( _desc.max ), build( _desc.step ), build( _desc.def ) );
}

}  // namespace librealsense
