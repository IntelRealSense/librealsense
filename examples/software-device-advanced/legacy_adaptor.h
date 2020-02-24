#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_H
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_H

#include "librealsense2/h/rs_internal.h"
#include "librealsense2/h/rs_types.h"

extern "C" {
    rs2_device * rs2_create_legacy_adaptor_device(const char * json, rs2_error** error);

    const char * rs2_legacy_adaptor_get_json_format();
}

#endif