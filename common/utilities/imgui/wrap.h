// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

namespace utilities {
namespace imgui {

// Wrap text according to input width
//     - Input:  - text as a string
//               - wrapping width
//     - Output: - on success - wrapped text
//               - on failure - an empty string
// Example:
//     Input:
//          this is the first line\nthis is the second line\nthis is the last line , wrap_width = 150 [pixels]
//     Output:
//          this is the\nfirst line\nthis is the\nsecond line\nthis is the last\nline
// Note: If the paragraph contain multiple spaces, it will be trimmed into a single space.
std::string wrap( const std::string & text, int wrap_pixels_width );

}  // namespace imgui
}  // namespace utilities
