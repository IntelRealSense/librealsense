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

TEST_CASE("SW update test [operator bool]")
{
    rs2::sw_update::version v1("01.02.03.04");
    CHECK(v1);

    rs2::sw_update::version v2("0.0.0.0");
    CHECK(!v2);

    rs2::sw_update::version v3(".2.3.4");
    CHECK(!v3);
}

TEST_CASE("SW update test [is_between]")
{
    rs2::sw_update::version v1("01.02.03.04");
    rs2::sw_update::version v2("01.03.03.04");
    rs2::sw_update::version v3("02.01.03.04");

    CHECK(v2.is_between(v1, v3));
    CHECK(v3.is_between(v1, v3));
    CHECK(v2.is_between(v2, v3));
    CHECK(v2.is_between(v2, v2));
}

TEST_CASE("SW update test [version with zeros]")
{
    rs2::sw_update::version v("01.02.03.04");
    std::string v_str = v;

    CHECK(v_str == "1.2.3.4");
}

TEST_CASE("SW update test [version invalid]")
{
    rs2::sw_update::version v(".2.3.4");
    std::string v_str = v;

    CHECK(v_str == "0.0.0.0");

    rs2::sw_update::version v1("1..3.4");
    v_str = v1;

    CHECK(v_str == "0.0.0.0");

    rs2::sw_update::version v2("1.5000.3.4");
    v_str = v1;

    CHECK(v_str == "0.0.0.0");

    rs2::sw_update::version v3("14567.2.3.4");
    v_str = v3;

    CHECK(v_str == "0.0.0.0");

    rs2::sw_update::version v4("1.2.3.400000");
    v_str = v4;

    CHECK(v_str == "0.0.0.0");
}