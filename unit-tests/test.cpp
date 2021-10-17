// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "test.h"


#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

namespace test {


std::string context;
bool debug = false;
debug_logger log;


log_message::~log_message()
{
    std::cout << '-' << char(_type) << "- " << _line.str() << std::endl;
}

}
