// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

// Generic, reusable composite (multi-field, atomically-exchanged) XU control - NOT named after
// any specific feature. Any future multi-param XU control can reuse this class; the only
// per-feature footprint is the (extension_unit, ctrl_id, wire_size) triple passed to the ctor.
//
// Implements ONLY composite_option_interface - zero relationship to librealsense::option (no
// shared base). No set(float)/query(), so it's never mistaken for a scalar option by code
// walking options_container::get_supported_options().
//
// get_raw()/set_raw()/get_raw_range() do the wire transaction directly via the locked
// uvc_sensor's invoke_powered() - uvc_xu_option<T>'s set/query are hardwired to sizeof(T), not
// this control's caller-supplied wire_size, so wrapping it would have bought nothing here.

#pragma once

#include <src/composite-option-interface.h>
#include <src/uvc-sensor.h>
#include <src/platform/uvc-device.h>

#include <memory>
#include <string>
#include <cstdint>
#include <vector>


namespace librealsense {

class composite_xu_option : public composite_option_interface
{
public:
    composite_xu_option( std::weak_ptr< uvc_sensor > ep,
                         platform::extension_unit xu,
                         uint8_t ctrl_id,
                         uint32_t wire_size,
                         std::string description );

    // composite_option_interface: EXACTLY one get_xu()/set_xu() call - the whole payload
    // travels atomically. This is the non-negotiable HW/FW invariant this class exists for.
    std::vector< uint8_t > get_raw() const override;
    void set_raw( const void * data, size_t size ) override;

    // One get_xu_range() call (a read-only metadata query, not subject to the get_raw/set_raw
    // atomicity contract above) - packs {version=1, min, max, step, def} per the generic
    // convention documented on composite_option_interface::get_raw_range().
    std::vector< uint8_t > get_raw_range() const override;

    bool is_enabled() const override { return true; }
    bool is_read_only() const override { return false; }
    const char * get_description() const override { return _description.c_str(); }

private:
    std::weak_ptr< uvc_sensor > _ep;
    platform::extension_unit _xu;
    uint8_t _ctrl_id;
    uint32_t _wire_size;
    std::string _description;
};

}  // namespace librealsense
