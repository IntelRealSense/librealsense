// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

// A standalone interface backing a "composite option": a multi-field control exchanged
// atomically (see rs_composite_option.h). NOT related to librealsense::option/option_interface -
// no set(float)/query(), so it's structurally unreachable through the scalar-option registry.
//
// Composite options live in their own registry, keyed by rs2_composite_option_id, on the SAME
// options_container that also holds ordinary rs2_option-keyed options - two separate maps, one class.

#pragma once

#include <librealsense2/h/rs_types.h>

#include <vector>
#include <cstdint>
#include <cstddef>

namespace librealsense {

class composite_option_interface
{
public:
    virtual ~composite_option_interface() = default;

    // Each call performs EXACTLY ONE UVC control transaction (one set_xu/get_xu). The bytes are
    // an opaque blob whose layout is documented on the corresponding rs2_composite_option_id (see
    // rs_composite_option.h / rs_temporal_filter_dpp.h).
    //
    // get_raw returns a vector sized exactly to this option's known wire size - the caller has
    // no generic way to know that size in advance, so the SDK owns the allocation (mirrors
    // librealsense::safety_sensor::get_safety_preset).
    virtual std::vector< uint8_t > get_raw() const = 0;
    virtual void set_raw( const void * data, size_t size ) = 0;

    // Backs rs2_get_composite_option_range. Generic convention: four back-to-back payloads, each
    // the option's normal wire size - min, max, step, def, in that order. No version field: a
    // range is always freshly read and returned whole, never hand-built and sent back.
    virtual std::vector< uint8_t > get_raw_range() const = 0;

    // Real, meaningful metadata for a composite control, just as it is for a scalar option -
    // declared fresh here rather than inherited, since this interface has no relationship to
    // librealsense::option.
    virtual bool is_read_only() const = 0;
    virtual bool is_enabled() const = 0;
    virtual const char * get_description() const = 0;
};

}  // namespace librealsense
