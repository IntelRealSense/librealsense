// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include "common.h"
#include <rsutils/string/trim-newlines.h>

using namespace rsutils::string;

TEST_CASE( "trim-newlines", "[string]" )
{
    // No changes because nothing behind
    CHECK( trim_newlines( "" ) == "" );
    CHECK( trim_newlines( "\n" ) == "\n" );

    // Put something behind...
    CHECK( trim_newlines( "a" ) == "a" );
    CHECK( trim_newlines( "\na" ) == "\na" );
    CHECK( trim_newlines( " \n " ) == "\n " );
    CHECK( trim_newlines( "a\n" ) == "a\n" );  // nothing in front
    CHECK( trim_newlines( "a\nb" ) == "a b" );
    CHECK( trim_newlines( "a   \nb" ) == "a b" );  // remove spaces at EOL

    // Multiple NLs should not get removed
    CHECK( trim_newlines( "a\n\nb" ) == "a\n\nb" );
    CHECK( trim_newlines( "a\n\n \nb" ) == "a\n\n\nb" );

    // Colons followed by NL should stay
    CHECK( trim_newlines( "line 1.\nline 2" ) == "line 1. line 2" );
    CHECK( trim_newlines( "line 1:\nline 2" ) == "line 1:\nline 2" );
    CHECK( trim_newlines( "line 1:  \nline 2" ) == "line 1:\nline 2" );
}
