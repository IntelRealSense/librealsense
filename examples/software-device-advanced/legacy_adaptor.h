/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_H
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_H

#include "librealsense2/h/rs_internal.h"
#include "librealsense2/h/rs_types.h"

extern "C" {
    /**
     * Create a librealsense2 software device that wraps a legacy librealsense1 device
     * \param[in]  runtime_api_version  API version of runtime, to ensure compatability
     * \param[in]  idx                  which legacy device to create, 0-indexed
     * \param[out] error                if non-null, receives any error that occurs during this call, otherwise, errors are ignored
     * \return                          software device object, should be released by rs2_delete_device
     */
    rs2_device * rs2_create_legacy_adaptor_device(int runtime_api_version, int idx, rs2_error** error);
}

#endif