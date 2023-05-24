// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


namespace rsutils {
namespace os {


// In a Windows application, console output does not automatically get relfected in a console, even if run from a
// console. Calling this will ensure a console is open (opening one in a window if none is available) and std::cout etc.
// are directed to it.
//
void ensure_console( bool create_if_none = true );


}
}
