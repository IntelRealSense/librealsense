// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <iostream>
#include "test.h"

namespace test {


std::string context;
bool debug = false;
debug_logger log;


log_message::~log_message()
{
    std::cout << '-' << char(_type) << "- " << _line.str() << std::endl;
}

}
