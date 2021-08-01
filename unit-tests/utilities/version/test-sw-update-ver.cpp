// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "common.h"
#include <common/sw-update/versions-db-manager.h>


TEST_CASE("sw_update version string ctor")
{
    rs2::sw_update::version v("1.2.3.4");
    std::string v_str = v;

    CHECK(v_str == "1.2.3.4");
}

//////////////////////// TEST Description ////////////////////
// Tests the version structure operators //
//////////////////////////////////////////////////////////////
TEST_CASE("SW update test [version operator >]")
{
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("0.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.1.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.2.2.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.2.3.3"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("2.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.3.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.2.4.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.2.3.5")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") > rs2::sw_update::version("1.2.3.4")));
}
TEST_CASE("SW update test [version operator <]")
{
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("2.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.3.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.2.4.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.2.3.5"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("0.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.1.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.2.2.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.2.3.3")));

    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") < rs2::sw_update::version("1.2.3.4")));
}
TEST_CASE("SW update test [version operator ==]")
{
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") == rs2::sw_update::version("1.2.3.4"));
    REQUIRE(rs2::sw_update::version("0.0.0.10") == rs2::sw_update::version("0.0.0.10"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") == rs2::sw_update::version("0.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") == rs2::sw_update::version("1.1.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") == rs2::sw_update::version("1.2.2.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") == rs2::sw_update::version("1.2.3.3")));
}
TEST_CASE("SW update test [version operator !=]")
{
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") != rs2::sw_update::version("0.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") != rs2::sw_update::version("1.1.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") != rs2::sw_update::version("1.2.2.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") != rs2::sw_update::version("1.2.3.3"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") != rs2::sw_update::version("1.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("0.0.0.10") != rs2::sw_update::version("0.0.0.10")));
}

TEST_CASE("SW update test [version operator >=]")
{
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") >= rs2::sw_update::version("1.2.3.4"));

    REQUIRE(rs2::sw_update::version("2.2.3.4") >= rs2::sw_update::version("1.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.3.3.4") >= rs2::sw_update::version("1.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.4.4") >= rs2::sw_update::version("1.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.5") >= rs2::sw_update::version("1.2.3.4"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") >= rs2::sw_update::version("2.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") >= rs2::sw_update::version("1.3.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") >= rs2::sw_update::version("1.2.4.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.4") >= rs2::sw_update::version("1.2.3.5")));
}

TEST_CASE("SW update test [version operator <=]")
{
    /////////////////////// Test operator <= /////////////////////
    // Verify success
    REQUIRE(rs2::sw_update::version("1.2.3.4") <= rs2::sw_update::version("1.2.3.4"));

    REQUIRE(rs2::sw_update::version("1.2.3.4") <= rs2::sw_update::version("2.2.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") <= rs2::sw_update::version("1.3.3.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") <= rs2::sw_update::version("1.2.4.4"));
    REQUIRE(rs2::sw_update::version("1.2.3.4") <= rs2::sw_update::version("1.2.3.5"));

    // Verify failure
    REQUIRE(false == (rs2::sw_update::version("2.2.3.4") <= rs2::sw_update::version("1.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.3.3.4") <= rs2::sw_update::version("1.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.4.4") <= rs2::sw_update::version("1.2.3.4")));
    REQUIRE(false == (rs2::sw_update::version("1.2.3.5") <= rs2::sw_update::version("1.2.3.4")));
}