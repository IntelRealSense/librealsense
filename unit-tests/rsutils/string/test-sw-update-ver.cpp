// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "common.h"
#include <common/sw-update/versions-db-manager.h>

using rs2::sw_update::version;


TEST_CASE( "string ctor" )
{
    CHECK_NOTHROW( version( "1.2.3.4" ));
    CHECK_NOTHROW( version( "xxxxxxxxxxx" ));
}

TEST_CASE( "default ctor" )
{
    CHECK_NOTHROW( version() );
}

TEST_CASE( "long long ctor" )
{
    CHECK( version( 0 ) == version() );

    CHECK( version( 102031234LL ).mjor == 1 );
    CHECK( version( 102031234LL ).mnor == 2 );
    CHECK( version( 102031234LL ).patch == 3 );
    CHECK( version( 102031234LL ).build == 1234 );

    CHECK( version( 102031234LL ) == version( "1.2.3.1234" ) );
}

TEST_CASE( "validity" )
{
    CHECK( ! version( "" ) );
    CHECK( ! version() );

    CHECK(   version( "1.2.3.4" ));
    CHECK(   version( "0.2.3.4" ));
    CHECK(   version( "1.0.0.0" ));
    CHECK( ! version( "0.0.0.0" ));
    
    CHECK( ! version( "1 . 2.3.4" ) );
    CHECK( ! version( ".2.3.4" ) );
}

TEST_CASE( "equality" )
{
    CHECK( version() == version() );
    CHECK( version() == version(0) );
    CHECK( version() == version("") );
    CHECK( version() == version("123") );
    
    CHECK( version() != version( "1.2.3.4" ));
    CHECK_FALSE( version() == version( "1.2.3.4" ));

    CHECK(       version( "1.2.3.4" ) == version( "1.2.3.4" ));
    CHECK_FALSE( version( "1.2.3.4" ) != version( "1.2.3.4" ));

    CHECK_FALSE( version( "1.2.3.4" ) == version( "0.2.3.4" ) );
    CHECK(       version( "1.2.3.4" ) != version( "0.2.3.4" ) );
    CHECK_FALSE( version( "1.2.3.4" ) == version( "1.1.3.4" ) );
    CHECK(       version( "1.2.3.4" ) != version( "1.1.3.4" ) );
    CHECK_FALSE( version( "1.2.3.4" ) == version( "1.2.2.4" ) );
    CHECK(       version( "1.2.3.4" ) != version( "1.2.2.4" ) );
    CHECK_FALSE( version( "1.2.3.4" ) == version( "1.2.3.3" ) );
    CHECK(       version( "1.2.3.4" ) != version( "1.2.3.3" ) );
}

TEST_CASE( "optional build number" )
{
    CHECK( version( "1.2.3" ) );
    CHECK( version( "1.2.3" ) == version( 102030000 ) );
    CHECK( version( "1.2.3.1" ) == version( 102030001 ) );
    CHECK( version( "1.2.3" ) != version( "1.2.3.1" ) );
}

TEST_CASE( "with zeros" )
{
    CHECK( std::string( version( "01.02.03.04" )) == "1.2.3.4" );
}

TEST_CASE( "constraints" )
{
    CHECK( ! version(0) );
    CHECK(   version(1) );
    CHECK(   version( "0.0.0.0001" ));
    CHECK( ! version( "0.0.0.00001" ));
    CHECK(   version( "0.0.99.0" ));
    CHECK( ! version( "0.0.999.0" ));
    CHECK(   version( "0.99.0.0" ));
    CHECK( ! version( "0.999.0.0" ));
    CHECK(   version( "99.0.0.0" ));
    CHECK( ! version( "999.0.0.0" ));
}

TEST_CASE( "copy ctor" )
{
    version v;
    version v2( v );
    CHECK( ! v2 );

    version v3( "1.2.3.4" );
    CHECK( v3 );
    version v4( v3 );
    CHECK( v3 == v4 );
}

TEST_CASE( "operator >" )
{
    // Verify success
    REQUIRE(version("1.2.3.4") > version("0.2.3.4"));
    REQUIRE(version("1.2.3.4") > version("1.1.3.4"));
    REQUIRE(version("1.2.3.4") > version("1.2.2.4"));
    REQUIRE(version("1.2.3.4") > version("1.2.3.3"));

    // Verify failure
    REQUIRE(false == (version("1.2.3.4") > version("2.2.3.4")));
    REQUIRE(false == (version("1.2.3.4") > version("1.3.3.4")));
    REQUIRE(false == (version("1.2.3.4") > version("1.2.4.4")));
    REQUIRE(false == (version("1.2.3.4") > version("1.2.3.5")));
    REQUIRE(false == (version("1.2.3.4") > version("1.2.3.4")));
}

TEST_CASE( "operator <" )
{
    // Verify success
    REQUIRE(version("1.2.3.4") < version("2.2.3.4"));
    REQUIRE(version("1.2.3.4") < version("1.3.3.4"));
    REQUIRE(version("1.2.3.4") < version("1.2.4.4"));
    REQUIRE(version("1.2.3.4") < version("1.2.3.5"));

    // Verify failure
    REQUIRE(false == (version("1.2.3.4") < version("0.2.3.4")));
    REQUIRE(false == (version("1.2.3.4") < version("1.1.3.4")));
    REQUIRE(false == (version("1.2.3.4") < version("1.2.2.4")));
    REQUIRE(false == (version("1.2.3.4") < version("1.2.3.3")));

    REQUIRE(false == (version("1.2.3.4") < version("1.2.3.4")));
}

TEST_CASE( "operator >=" )
{
    // Verify success
    REQUIRE(version("1.2.3.4") >= version("1.2.3.4"));

    REQUIRE(version("2.2.3.4") >= version("1.2.3.4"));
    REQUIRE(version("1.3.3.4") >= version("1.2.3.4"));
    REQUIRE(version("1.2.4.4") >= version("1.2.3.4"));
    REQUIRE(version("1.2.3.5") >= version("1.2.3.4"));

    // Verify failure
    REQUIRE(false == (version("1.2.3.4") >= version("2.2.3.4")));
    REQUIRE(false == (version("1.2.3.4") >= version("1.3.3.4")));
    REQUIRE(false == (version("1.2.3.4") >= version("1.2.4.4")));
    REQUIRE(false == (version("1.2.3.4") >= version("1.2.3.5")));
}

TEST_CASE( "operator <=" )
{
    /////////////////////// Test operator <= /////////////////////
    // Verify success
    REQUIRE(version("1.2.3.4") <= version("1.2.3.4"));

    REQUIRE(version("1.2.3.4") <= version("2.2.3.4"));
    REQUIRE(version("1.2.3.4") <= version("1.3.3.4"));
    REQUIRE(version("1.2.3.4") <= version("1.2.4.4"));
    REQUIRE(version("1.2.3.4") <= version("1.2.3.5"));

    // Verify failure
    REQUIRE(false == (version("2.2.3.4") <= version("1.2.3.4")));
    REQUIRE(false == (version("1.3.3.4") <= version("1.2.3.4")));
    REQUIRE(false == (version("1.2.4.4") <= version("1.2.3.4")));
    REQUIRE(false == (version("1.2.3.5") <= version("1.2.3.4")));
}

TEST_CASE( "is_between" )
{
    version v1("01.02.03.04");
    version v2("01.03.03.04");
    version v3("02.01.03.04");

    CHECK(v2.is_between(v1, v3));
    CHECK(v3.is_between(v1, v3));
    CHECK(v2.is_between(v2, v3));
    CHECK(v2.is_between(v2, v2));
}
