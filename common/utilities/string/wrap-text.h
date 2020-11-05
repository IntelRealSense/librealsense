// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <sstream>

#include "../third-party/imgui/imgui.h"

namespace utilities {
namespace string {


// Split input test to lines
//     - Input: std:string text
//     - Output: a vector of (std::string) lines
// Example:
//     Input:
//          This is the first line\nThis is the second line\nThis is the last line
//     Output:
//          [0] this is the first line
//          [1] this is the second line
//          [2] this is the last line
inline std::vector< std::string > split_string_by_newline( const std::string & str )
{
    auto result = std::vector< std::string >{};
    auto ss = std::stringstream{ str };

    for( std::string line; std::getline( ss, line, '\n' ); )
        result.push_back( line );

    return result;
}

// Wrap text according to input width
//     - Input:  - text as a string
//               - wrapping width
//     - Output: - on success - wrapped text
//               - on failure - an empty string
// Example:
//     Input:
//          This is the first line\nThis is the second line\nThis is the last line , wrap_width = 15
//     Output:
//          this is the\nfirst line\nthis is the\nsecond line\nthis is the\nlast line
inline std::string wrap_text(const std::string & text, float wrap_pixels_width)
{
    // Do not wrap if the wrap is smaller then 32 pixels (~2 characters on font 16)
    if (wrap_pixels_width < 32.0f)
        return "";

    // split text into paragraphs
    auto lines_vector = split_string_by_newline(text);

    std::string wrapped_text;
    float space_size = ImGui::CalcTextSize(" ").x;
    bool first_line = true;

    // Wrap each line according to the requested wrap width
    for (auto line : lines_vector)
    {
        std::string current_line; // The line that is built in this iteration 
        auto temp_line = line;    // holds the input line and gets shorten each iteration
        auto next_word = temp_line.substr(0, temp_line.find(" ")); // The next word to add to the current line
        bool first_word = true;

        // each paragraph except the first one starts in a new line
        if (!first_line) current_line += '\n'; 

        while (!next_word.empty())
        {
            float next_pos = 0.0f;
            // If this is the first word we try to place it first in line, 
            // if not we concatenate it to the last word after adding a space
            if (!first_word)
            {
                next_pos = ImGui::CalcTextSize(current_line.c_str()).x + space_size;
            }

            if (next_pos + ImGui::CalcTextSize(next_word.c_str()).x <= wrap_pixels_width)
            {
                if (!first_word) current_line += " "; // first word should not start with " "
                current_line += next_word;            
            }
            else
            {  // current line cannot feat new word so we wrap the line and 
               // start building the new line

                wrapped_text += current_line;           // copy line build by now
                if (!first_word)  wrapped_text += '\n'; // break the previous line if exist
                current_line = next_word;               // add next work to new line
            }

            first_word = false;
            first_line = false;

            // If we have more characters left other then the current word, prepare inputs for next iteration
            // If not add the current line built so far to the output and finish current line wrap.
            if (temp_line.size() > next_word.size()) 
            {
                temp_line = temp_line.substr(next_word.size() + 1);
                next_word = temp_line.substr(0, temp_line.find(" "));
            }
            else
            {
                wrapped_text += current_line;
                break;
            }
        }
    }

    return wrapped_text;
}

}  // namespace string
}  // namespace utilities
