// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../common/utilities/imgui/wrap.h
//#cmake:add-file ../../../common/utilities/imgui/wrap.cpp
//#cmake:add-file ../../../third-party/imgui/imgui.h

#include "common.h"
#include "../../../common/utilities/imgui/wrap.h"
#include "../../../third-party/imgui/imgui.h"

namespace ImGui {
// Mock ImGui function for test
// all characters are at length of 10 pixels except 'i' and 'j' which is 5 pixels
ImVec2 CalcTextSize( const char * text,
                     const char * text_end,
                     bool hide_text_after_double_hash,
                     float wrap_width )
{
    if (!text) return ImVec2(0.0f, 0.0f);

    float total_size = 0.0f;
    while( *text )
    {
        if( *text == 'i' || *text == 'j' )
            total_size += 5.0f;
        else
            total_size += 10.0f;
        text++;
    }
    return ImVec2( total_size, 0.0f );
}
}  // namespace ImGui

using namespace utilities::imgui;

TEST_CASE( "wrap-text", "[string]" )
{
    // Verify illegal inputs return empty string
    CHECK( wrap( "", 0 ) == "" );
    CHECK( wrap( "", 10 ) == "" );
    CHECK( wrap( "abc", 0 ) == "abc" );
    CHECK( wrap( "abc", 10 ) == "abc" );
    CHECK( wrap( "abc\nabc", 0 ) == "abc\nabc" );
    CHECK( wrap( "abc abc", 5 ) == "abc abc" );
    CHECK( wrap( "", 10 ) == "" );

    // Verify no wrap if not needed
    CHECK( wrap( "abc", 100 ) == "abc" );
    CHECK( wrap( "abc\nabc", 100 ) == "abc\nabc" );
    CHECK( wrap( "abc abc a", 100 ) == "abc abc a" );

    // No wrapping possible, copy line until first space and continue wrapping
    CHECK( wrap( "abcdefgh", 40 ) == "abcdefgh" );
    CHECK( wrap( "aabbccddff das ds fr", 50 ) == "aabbccddff\ndas\nds fr" );
    CHECK( wrap( "das aabbccddff ds fr", 50 ) == "das\naabbccddff\nds fr" );

    // Exact wrap position test
    CHECK( wrap( "abcde abcde", 50 ) == "abcde\nabcde" );

    // Short letters test
    CHECK(wrap("aaaa bbbb cc", 100) == "aaaa bbbb\ncc");
    // i and j are only 5 pixels so we get more characters inside the wrap
    CHECK(wrap("aaaa iijj cc", 100) == "aaaa iijj cc"); 


    // Check wrapping of 3 paragraphs
    CHECK( wrap( "this is the first line\nthis is the second line\nthis is the last line", 150 )
           == "this is the\nfirst line\nthis is the\nsecond line\nthis is the last\nline" );

    CHECK( wrap( "this is the first line\nthis is the second line\nthis is the last line", 60 )
           == "this is\nthe\nfirst\nline\nthis is\nthe\nsecond\nline\nthis is\nthe\nlast\nline" );

    // Spaces checks
    CHECK(wrap("ab cd ", 32) == "ab\ncd");      // Ending spaces
    CHECK(wrap("ab  cd", 32) == "ab\ncd");      // Middle spaces
    CHECK(wrap(" ab  cd ", 32) == "ab\ncd");    // Mixed multiple spaces
    CHECK(wrap("  ab  cd ", 32) == "ab\ncd");   // Mixed multiple spaces
    CHECK(wrap("  ab    ", 33) == "ab");        // Mixed multiple spaces
    CHECK(wrap("ab    ", 33) == "ab");          // Ending multiple spaces
    CHECK(wrap("  ab", 33) == "ab");

    // Only spaces
    CHECK(wrap(" ", 33) == "");
    CHECK(wrap("  ", 33) == "");

    CHECK(wrap("ab cd ", 100) == "ab cd");
    CHECK(wrap("ab  cd", 100) == "ab cd"); // Known corner case - we trim multiple spaces

}
