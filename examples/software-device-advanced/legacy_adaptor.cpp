/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#include <librealsense2/rs.hpp> // Librealsense Cross-Platform API
#include <librealsense2/hpp/rs_internal.hpp> // Librealsense Software Device header

#include "api.h"
#include "legacy_adaptor.h"
#include "active_obj.hpp"

// legacy API needs singleton context, this is the best way to accomplish that.
static rs::context& get_context() { static rs::context ctx; return ctx; }

rs2_device * rs2_create_legacy_adaptor_device(int runtime_api_version, int idx, rs2_error** error) BEGIN_API_CALL {
    verify_version_compatibility(runtime_api_version);
    auto& legacy_ctx = get_context();
    VALIDATE_RANGE(idx, 0, legacy_ctx.get_device_count());

    // Create software device such that it doesn't delete the 
    // underlying C pointer so we can pass it upward to the application
    rs2::software_device sd([](rs2_device*) {});
    auto ao = std::make_shared<legacy_active_obj>(legacy_ctx, idx, sd);
    ao->finalize_binding(sd);

    // extract the raw pointer to rs2_device from the software_device
    return sd.get().get();
} HANDLE_EXCEPTIONS_AND_RETURN(nullptr, idx)