// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace rsutils {
namespace string {


// Split input text to vector of strings according to input delimiter
//     - Input: string to be split
//     -        delimiter
//     - Output: a vector of strings
// Example:
//     Input:
//          Text: This is the first line\nThis is the second line\nThis is the last line
//          Delimiter : '\n'
//     Output:
//          [0] this is the first line
//          [1] this is the second line
//          [2] this is the last line
inline std::vector< std::string > split( const std::string & str , char delimiter)
{
    auto result = std::vector< std::string >{};
    auto ss = std::stringstream{ str };

    for( std::string line; std::getline( ss, line, delimiter); )
        result.push_back( line );

    return result;
}

}  // namespace string
}  // namespace rsutils
