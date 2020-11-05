// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../third-party/imgui/imgui.h


#include "common.h"
#include "../../../common/utilities/string/wrap-text.h"

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

using namespace utilities::string;

TEST_CASE("split_string_by_newline", "[string]")
{
    // split size check
    CHECK(split_string_by_newline("").size() == 0);
    CHECK(split_string_by_newline("abc").size() == 1);
    CHECK(split_string_by_newline("abc\nabc").size() == 2);
    CHECK(split_string_by_newline("a\nbc\nabc").size() == 3);
    CHECK(split_string_by_newline("a\nbc\nabc\n").size() == 3);

    CHECK(split_string_by_newline("a\nbc\nabc")[0] == "a");
    CHECK(split_string_by_newline("a\nbc\nabc")[1] == "bc");
    CHECK(split_string_by_newline("a\nbc\nabc")[2] == "abc");
    CHECK(split_string_by_newline("a\nbc\nabc\n")[2] == "abc");
}


TEST_CASE( "wrap-text", "[string]" )
{
    // Verify illegal inputs return empty string
    CHECK( wrap_text( "", 0 ) == "" );
    CHECK( wrap_text( "", 10 ) == "" );
    CHECK( wrap_text( "abc", 0 ) == "" );
    CHECK( wrap_text( "abc", 10 ) == "" );
    CHECK( wrap_text( "abc\nabc", 0 ) == "" );
    CHECK( wrap_text( "abc\nabc", 10 ) == "" );
    CHECK( wrap_text( "", 10 ) == "" );

    // Verify no wrap if not needed
    CHECK( wrap_text( "abc", 100 ) == "abc" );
    CHECK( wrap_text( "abc\nabc", 100 ) == "abc\nabc" );
    CHECK( wrap_text( "abc abc a", 100 ) == "abc abc a" );

    // No wrapping possible, copy line until first space and continue wrapping
    CHECK( wrap_text( "abcdefgh", 40 ) == "abcdefgh" );
    CHECK( wrap_text( "aabbccddff das ds fr", 50 ) == "aabbccddff\ndas\nds fr" );
    CHECK( wrap_text( "das aabbccddff ds fr", 50 ) == "das\naabbccddff\nds fr" );

    // Exact wrap position test
    CHECK( wrap_text( "abcde abcde", 50 ) == "abcde\nabcde" );

    // Short letters test
    CHECK(wrap_text("aaaa bbbb cc", 100) == "aaaa bbbb\ncc");
    // i and j are only 5 pixels so we get more characters inside the wrap
    CHECK(wrap_text("aaaa iijj cc", 100) == "aaaa iijj cc"); 


    // Check wrapping of 3 paragraphs
    CHECK( wrap_text( "this is the first line\nthis is the second line\nthis is the last line", 150 )
           == "this is the\nfirst line\nthis is the\nsecond line\nthis is the\nlast line" );

    CHECK( wrap_text( "this is the first line\nthis is the second line\nthis is the last line", 60 )
           == "this is\nthe\nfirst\nline\nthis\nis the\nsecond\nline\nthis\nis the\nlast\nline" );
}
