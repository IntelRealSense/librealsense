// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-4 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/extension.h"
#include <vector>
#include <string>


namespace librealsense {


class serializable_interface
{
public:
    virtual ~serializable_interface() {}

    virtual std::vector< uint8_t > serialize_json() const = 0;
    virtual void load_json( const std::string & json_content ) = 0;
};

MAP_EXTENSION( RS2_EXTENSION_SERIALIZABLE, serializable_interface );


}  // namespace librealsense
