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
// This is an abstract base: it owns `invoke_powered`, the size checks, and the min|max|step|def
// packing, and delegates the actual wire transaction to three pure-virtual hooks
// (`read_from_device`/`write_to_device`/`read_range_from_device`). Two concrete subclasses
// exist today - `composite_usb_xu_option` (plain uvc XU passthrough - FW authors the
// dpp_header on get/range, app carries it on set) and `composite_mipi_xu_option` (same
// passthrough for get/set - the driver pipes the whole struct - but the range comes from
// static tables since neither V4L2 nor the GMSL FW register map can provide it).
// Callers always go through the `create()` factory below; nothing outside this file picks
// the subclass directly.

#pragma once

#include <src/composite-option-interface.h>
#include <src/uvc-sensor.h>
#include <src/platform/uvc-device.h>

#include <memory>
#include <string>
#include <cstdint>
#include <vector>


namespace librealsense {

// Describes a DPP composite XU control - the same facts both transports need to describe the
// wire (ctl_id / param_count / param_type + per-slot ranges). USB doesn't consume these
// (FW authors them into the payload); MIPI uses them only to synthesize the range, as GMSL
// has no way to query it. The description belongs to the control itself, not to the
// transport - hence one struct shared by both branches of `create()`.
struct dpp_control_desc
{
    uint16_t ctl_id;         // dpp_header.ctl_id of the synthesized range (MIPI-only)
    uint8_t  param_count;    // active int32 slots
    uint8_t  param_type;     // dpp_header.param_type of the synthesized range (MIPI-only)
    const int32_t * min;     // param_count entries each
    const int32_t * max;
    const int32_t * step;
    const int32_t * def;
};

class composite_xu_option : public composite_option_interface
{
public:
    composite_xu_option( std::weak_ptr< uvc_sensor > ep,
                         platform::extension_unit xu,
                         uint8_t ctrl_id,
                         uint32_t wire_size,
                         std::string description );

    virtual ~composite_xu_option() = default;

    // Single place that picks the right concrete subclass. `desc` is unused on the USB branch
    // (FW authors those fields into the payload) - callers pass it unconditionally so the
    // description of the control lives beside the struct it mirrors, not the transport.
    static std::shared_ptr< composite_xu_option > create( bool is_mipi,
                                                          std::weak_ptr< uvc_sensor > ep,
                                                          platform::extension_unit xu,
                                                          uint8_t ctrl_id,
                                                          uint32_t wire_size,
                                                          std::string description,
                                                          const dpp_control_desc & desc );

    // composite_option_interface: EXACTLY one get_xu()/set_xu() call - the whole payload
    // travels atomically. This is the non-negotiable HW/FW invariant this class exists for.
    std::vector< uint8_t > get_raw() const final;
    void set_raw( const void * data, size_t size ) final;

    // One get_xu_range() call (a read-only metadata query, not subject to the get_raw/set_raw
    // atomicity contract above) - packs {version=1, min, max, step, def} per the generic
    // convention documented on composite_option_interface::get_raw_range().
    std::vector< uint8_t > get_raw_range() const final;

    bool is_enabled() const final { return true; }
    bool is_read_only() const final { return false; }
    const char * get_description() const final { return _description.c_str(); }

protected:
    // Pure-virtual extension points - subclasses do the wire transaction. Called with the
    // endpoint already locked and powered.
    virtual std::vector< uint8_t > read_from_device( platform::uvc_device & dev ) const = 0;
    virtual void write_to_device( platform::uvc_device & dev, const void * data, size_t size ) const = 0;
    virtual platform::control_range read_range_from_device( platform::uvc_device & dev ) const = 0;

    const platform::extension_unit & xu() const { return _xu; }
    uint8_t ctrl_id() const { return _ctrl_id; }
    uint32_t wire_size() const { return _wire_size; }

private:
    std::weak_ptr< uvc_sensor > _ep;
    platform::extension_unit _xu;
    uint8_t _ctrl_id;
    uint32_t _wire_size;
    std::string _description;
};

}  // namespace librealsense
