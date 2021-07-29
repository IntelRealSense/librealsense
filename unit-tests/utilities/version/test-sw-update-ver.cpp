// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "common.h"
#include "../../../common/sw-update/versions-db-manager.h"


TEST_CASE("sw_update version string ctor", "[string]")
{
    rs2::sw_update::version v("1.2.3.4");
    std::string v_str = v;

    CHECK(v_str == "1.2.3.4");
}

TEST_CASE("sw_update version operator<=", "[string]")
{
    rs2::sw_update::version v1("1.2.3.4");
    rs2::sw_update::version v2("1.2.3.5");
    rs2::sw_update::version v3("1.2.4.4");
    rs2::sw_update::version v4("1.3.3.4");
    rs2::sw_update::version v5("2.2.3.4");

    CHECK(v1 <= v2);
    CHECK(v1 <= v3);
    CHECK(v1 <= v4);
    CHECK(v1 <= v5);
}