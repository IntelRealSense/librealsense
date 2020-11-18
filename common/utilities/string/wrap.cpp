// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <string>
#include <vector>
#include <sstream>
#include "wrap.h"
#include "split.h"
#include "../third-party/imgui/imgui.h"

namespace utilities {
namespace string {

std::string wrap_paragraph( const std::string & paragraph, int wrap_pixels_width )
{
    float space_size = ImGui::CalcTextSize( " " ).x;  // Calculate space width in pixels
    std::string wrapped_line;                         // The line that is wrapped in this iteration
    std::string wrapped_paragraph;                    // The output wrapped paragraph
    auto remaining_paragraph = paragraph;             // holds the remaining unwrapped part of the input paragraph
    auto next_word = remaining_paragraph.substr(
        0,
        remaining_paragraph.find( " " ) );  // The next word to add to the current line
    bool first_word = true;

    while( ! next_word.empty() )
    {
        float next_x = 0.0f;
        // If this is the first word we try to place it first in line,
        // if not we concatenate it to the last word after adding a space
        if( ! first_word )
        {
            next_x = ImGui::CalcTextSize( wrapped_line.c_str() ).x + space_size;
        }

        if( next_x + ImGui::CalcTextSize( next_word.c_str() ).x <= wrap_pixels_width )
        {
            if( ! first_word )
                wrapped_line += " ";  // first word should not start with " "
            wrapped_line += next_word;
        }
        else
        {  // current line cannot feat new word so we wrap the line and
           // start building the new line

            wrapped_paragraph += wrapped_line;  // copy line build by now
            if( ! first_word )
                wrapped_paragraph += '\n';      // break the previous line if exist
            wrapped_line = next_word;           // add next work to new line
        }

        first_word = false;

        // If we have more characters left other then the current word, prepare inputs for next
        // iteration, If not add the current line built so far to the output and finish current
        // line wrap.
        if( remaining_paragraph.size() > next_word.size() )
        {
            remaining_paragraph = remaining_paragraph.substr( next_word.size() + 1 );
            next_word = remaining_paragraph.substr( 0, remaining_paragraph.find( " " ) );
        }
        else
        {
            wrapped_paragraph += wrapped_line;
            break;
        }
    }

    return wrapped_paragraph;
}

std::string wrap( const std::string & text, int wrap_pixels_width )
{
    // Do not wrap if the wrap is smaller then 32 pixels (~2 characters on font 16)
    if( wrap_pixels_width < 32 )
        return text;

    // split text into paragraphs
    auto paragraphs_vector = split( text, '\n' );

    std::string wrapped_text;
    int line_number = 1;
    // Wrap each line according to the requested wrap width
    for( auto paragraph : paragraphs_vector )
    {
        wrapped_text += wrap_paragraph( paragraph, wrap_pixels_width );
        // each paragraph except the last one ends with a new line
        if( line_number++ != paragraphs_vector.size() )
            wrapped_text += '\n';
    }

    return wrapped_text;
}

}  // namespace string
}  // namespace utilities
