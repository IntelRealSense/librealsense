// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

// MIPI/GMSL concrete of composite_xu_option. The driver passes the whole 38-byte
// {dpp_header + 8 int32 slots} struct between FW and the SDK on get/set, exactly like USB, so
// the I/O hooks are inherited from composite_usb_xu_option. The only MIPI-specific part is the
// range: V4L2 cannot report per-slot min/max/step/def for an array control and the GMSL FW
// register map has no range window, so the range is synthesized from the caller-supplied
// `dpp_control_desc` (static tables in each *-filter-feature.cpp). Instantiated by
// composite_xu_option::create() on the MIPI branch; nothing else constructs it directly.
#pragma once

#include "composite-usb-xu-option.h"

#include <librealsense2/h/rs_dpp_header.h>


namespace librealsense {

class composite_mipi_xu_option : public composite_usb_xu_option
{
public:
    composite_mipi_xu_option( std::weak_ptr< uvc_sensor > ep,
                              platform::extension_unit xu,
                              uint8_t ctrl_id,
                              uint32_t wire_size,
                              std::string description,
                              const dpp_control_desc & desc );

protected:
    platform::control_range read_range_from_device( platform::uvc_device & dev ) const override;

private:
    // Stored by value so a caller-side temporary desc can't dangle - the caller-side pointer
    // arrays (min/max/step/def) still need static lifetime, which today's `static constexpr`
    // tables in each *_filter_feature.cpp satisfy.
    dpp_control_desc _desc;
};

}  // namespace librealsense
