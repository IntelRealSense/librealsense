// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/ds/features/amplitude-factor-feature.h>


namespace librealsense {

/* static */ const feature_id amplitude_factor_feature::ID = "Amplitude factor feature";

feature_id amplitude_factor_feature::get_id() const
{
    return ID;
}

}  // namespace librealsense
