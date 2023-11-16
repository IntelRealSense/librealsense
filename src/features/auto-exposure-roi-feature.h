// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>

#include <string>


namespace librealsense {

class auto_exposure_roi_feature : public feature_interface
{
public:
    static constexpr const char * ID = "Auto exposure ROI feature";

    auto_exposure_roi_feature() : feature_interface( ID )
    {
    }
};

}  // namespace librealsense
