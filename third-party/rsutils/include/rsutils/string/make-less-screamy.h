// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/ios/word-format.h>


namespace rsutils {
namespace string {


// Convert a typically all-uppercase, underscore-delimited identifier (e.g., "PRODUCT_LINE_D500") to a more
// human-readable form (e.g., "Product Line D500").
// 
//      - underscores are replaced by spaces
//      - letters are made lower-case; the beginning of words upper-case
//
inline std::string make_less_screamy( char const * c_str )
{
    return rsutils::ios::word_format( c_str )
        .delimited_by( "_", rsutils::ios::word_format::rf_space )
        .capitalize_first_letter()
        .str();
}


inline std::string make_less_screamy( std::string const & str )
{
    return make_less_screamy( str.c_str() );
}


}  // namespace string
}  // namespace rsutils
