// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/ds/features/remove-ir-pattern-feature.h>


namespace librealsense {

/* static */ const feature_id remove_ir_pattern_feature::ID = "Remove IR pattern feature";

feature_id remove_ir_pattern_feature::get_id() const
{
    return ID;
}

}  // namespace librealsense
