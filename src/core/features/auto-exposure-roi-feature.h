// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/features/feature-interface.h>

#include <string>


namespace librealsense {

class auto_exposure_roi_feature : public feature_interface
{
public:
    auto_exposure_roi_feature() : feature_interface( "Auto exposure ROI feature" )
    {
    }
};

}  // namespace librealsense
