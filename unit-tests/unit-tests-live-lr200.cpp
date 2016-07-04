// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>

TEST_CASE("Streams COLOR HD 1920X1080  Raw16 30FPS", "[live] [lr200] [one-camera]")
{
    test_ds_device_streaming({ {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW16, 30} });
}
