// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "pipeline.h"

using namespace librealsense;

pipeline::pipeline()
{
    _dev = _hub.wait_for_device();
}
