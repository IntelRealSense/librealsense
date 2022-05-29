// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../include/librealsense2/utilities/string/split.h

#include "common.h"
#include <librealsense2/utilities/string/split.h>

using namespace utilities::string;

TEST_CASE("split_string_by_newline", "[string]")
{
    // split size check
    CHECK(split("" , '\n').size() == 0);
    CHECK(split("abc", '\n').size() == 1);
    CHECK(split("abc\nabc", '\n').size() == 2);
    CHECK(split("a\nbc\nabc", '\n').size() == 3);
    CHECK(split("a\nbc\nabc\n", '\n').size() == 3);
    CHECK(split("1-12-123-1234", '-').size() == 4);

    CHECK(split("a\nbc\nabc", '\n')[0] == "a");
    CHECK(split("a\nbc\nabc", '\n')[1] == "bc");
    CHECK(split("a\nbc\nabc", '\n')[2] == "abc");
    CHECK(split("a\nbc\nabc\n", '\n')[2] == "abc");


    CHECK(split("1-12-123-1234", '-')[0] == "1");
    CHECK(split("1-12-123-1234", '-')[1] == "12");
    CHECK(split("1-12-123-1234", '-')[2] == "123");
    CHECK(split("1-12-123-1234", '-')[3] == "1234");
}
