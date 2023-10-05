// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <string>
#include <vector>
#include <sstream>
#include "wrap.h"
#include <rsutils/string/split.h>
#include <third-party/imgui/imgui.h>

namespace utilities {
namespace imgui {

void trim_leading_spaces( std::string &remaining_paragraph )
{
    auto non_space_index = remaining_paragraph.find_first_not_of( ' ' );

    if( non_space_index != std::string::npos )
    {
        // Remove trailing spaces
        remaining_paragraph = remaining_paragraph.substr( non_space_index );
    }
}

std::string wrap_paragraph( const std::string & paragraph, int wrap_pixels_width )
{
    float space_width = ImGui::CalcTextSize( " " ).x;  // Calculate space width in pixels
    std::string wrapped_line;                          // The line that is wrapped in this iteration
    std::string wrapped_paragraph;                     // The output wrapped paragraph
    auto remaining_paragraph
        = paragraph;  // Holds the remaining unwrapped part of the input paragraph

    // Handle a case when the paragraph starts with spaces
    trim_leading_spaces(remaining_paragraph);

    auto next_word = remaining_paragraph.substr(
        0,
        remaining_paragraph.find( ' ' ) );  // The next word to add to the current line
    bool first_word = true;

    while( ! next_word.empty() )
    {
        float next_x = 0.0f;
        // If this is the first word we try to place it first in line,
        // if not we concatenate it to the last word after adding a space
        if( ! first_word )
        {
            next_x = ImGui::CalcTextSize( wrapped_line.c_str() ).x + space_width;
        }

        if( next_x + ImGui::CalcTextSize( next_word.c_str() ).x <= wrap_pixels_width )
        {
            if( ! first_word )
                wrapped_line += " ";  // First word should not start with " "
            wrapped_line += next_word;
        }
        else
        {  // Current line cannot feat new word so we wrap the line and
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

            // Handle a case when the paragraph starts with spaces
            trim_leading_spaces(remaining_paragraph);

            next_word = remaining_paragraph.substr( 0, remaining_paragraph.find( ' ' ) );

            // If no more words exist, copy the current wrapped line to output and stop
            if( next_word.empty() )
            {
                wrapped_paragraph += wrapped_line;
                break;
            }
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

    // Split text into paragraphs
    auto paragraphs_vector = rsutils::string::split( text, '\n' );

    std::string wrapped_text;
    size_t line_number = 1;
    // Wrap each line according to the requested wrap width
    for( auto paragraph : paragraphs_vector )
    {
        wrapped_text += wrap_paragraph( paragraph, wrap_pixels_width );
        // Each paragraph except the last one ends with a new line
        if( line_number++ != paragraphs_vector.size() )
            wrapped_text += '\n';
    }

    return wrapped_text;
}

}  // namespace imgui
}  // namespace utilities
