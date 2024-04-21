// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace rsutils {
namespace string {


// Convert a typically all-uppercase, underscore-delimited identifier (e.g., "PRODUCT_LINE_D500") to a more
// human-readable form (e.g., "Product Line D500").
// 
//      - underscores are replaced by spaces
//      - letters are made lower-case; the beginning of a words upper-case
//
std::string make_less_screamy( std::string );


}  // namespace string
}  // namespace rsutils
