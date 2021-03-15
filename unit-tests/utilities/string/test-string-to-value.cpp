// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../common/realsense-ui-advanced-mode.h

#include "common.h"
#include "../../../common/realsense-ui-advanced-mode.h"


TEST_CASE("string to value", "[string]")
{
    int value;
    CHECK(string_to_value<int>(std::string("12334"), value));
    CHECK(value == 12334);
}
