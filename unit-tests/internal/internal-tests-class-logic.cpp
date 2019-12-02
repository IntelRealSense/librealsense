// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include <cmath>
#include <iostream>
#include "./../src/api.h"

TEST_CASE("verify_version_compatibility", "[code]")
{
    rs2_error* e = 0;
    rs2_context* ctx=nullptr;

    auto runtime_api_version = rs2_get_api_version(&e);
    CAPTURE(runtime_api_version);
    REQUIRE(!e);

    int api_ver = RS2_API_VERSION;
    int major = (api_ver / 10000);
    int minor = (api_ver / 100) % 100;
    int patch = api_ver  % 100;

    REQUIRE_NOTHROW(verify_version_compatibility(api_ver));
    //// Differences in major shall always result in error
    REQUIRE_THROWS(verify_version_compatibility(api_ver + 10000));
    REQUIRE_THROWS(verify_version_compatibility(api_ver - 10000));

    // Differences in minor are compatible as long as exe_ver <= lib_ver
    if (minor > 0)
    {
        REQUIRE_NOTHROW(verify_version_compatibility(api_ver - 100));
    }
    if (minor < 99)
    {
        REQUIRE_THROWS(verify_version_compatibility(api_ver + 100));
    }

    // differences in patch version do not affect compatibility
    int base = api_ver - patch;
    for (int i = 0; i < 100; i++)
    {
        CAPTURE(base + i);
        REQUIRE_NOTHROW(verify_version_compatibility(base+i));
    }
}
