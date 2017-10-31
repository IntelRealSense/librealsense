//
// Created by konrad on 10/24/17.
//

#ifndef PROJECT_ALIGN_Z_TO_OTHER_H
#define PROJECT_ALIGN_Z_TO_OTHER_H
#include "librealsense/rs.h"     // Inherit all type definitions in the public API

namespace gpu {
    bool align_z_to_other(rsimpl::byte * z_aligned_to_other, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, const rs_extrinsics & z_to_other, const rs_intrinsics & other_intrin);
}


#endif //PROJECT_ALIGN_Z_TO_OTHER_H
