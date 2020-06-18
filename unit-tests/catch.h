// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// Catch defines CHECK() and so does EL++, and so we have to undefine it or we get compilation errors!
// If EL++ was already defined:
#undef CHECK
// Otherwise, don't let it define its own:
#define ELPP_NO_CHECK_MACROS

#include "catch/catch.hpp"

