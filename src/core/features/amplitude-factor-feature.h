// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/features/feature-interface.h>

#include <string>


namespace librealsense {

class amplitude_factor_feature : public feature_interface
{
public:
    amplitude_factor_feature() : feature_interface( "Amplitude factor feature" )
    {
    }
};

}  // namespace librealsense
