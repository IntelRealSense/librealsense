// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "ellipsis.h"


namespace rsutils {
namespace string {


// Shorten the given json string representation so it does not exceed a maximum length.
//
// This is a formatting function - mostly used in reports, debugging output, etc.
// E.g.:
//      {"one":1,"two":2,"three":3,"four":4} -> {"one":1,"two":2,"thr ... }
//      {"one":1,"two":[1,2,3],"three":{"longassblock":{"insideblock":89012}},"four":4}
//          -> {"one":1,"two":[1,2,3],"three":{"longassblock":{ ... }},"four":4}
//          -> {"one":1,"two":[1,2,3],"three":{ ... },"four":4}
// 
// Both [] and {} blocks are considered for shortening.
//
// When more than one inside block exists, all are evaluated recursively to find the longest
// representation possible.
// 
// @param max_length
//      must be reasonably long (at least enough characters to encompass '{ ... }' or 7 characters) or the original
//      string will be returned
//
ellipsis shorten_json_string( slice const & str, size_t max_length = 96 );


}  // namespace string
}  // namespace rsutils
