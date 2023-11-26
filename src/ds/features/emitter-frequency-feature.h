// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


#include <src/feature-interface.h>


namespace librealsense {

class synthetic_sensor;

class emitter_frequency_feature : public feature_interface
{
public:
    static const feature_id ID;

    emitter_frequency_feature( synthetic_sensor & raw_sensor );

    feature_id get_id() const override;
};


}  // namespace librealsense
