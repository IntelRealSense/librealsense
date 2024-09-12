// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// Catch defines CHECK() and so does EL++, and so we have to undefine it or we get compilation errors!
#undef CHECK
// Otherwise, don't let it define its own:
#define ELPP_NO_CHECK_MACROS

// We set our own main by default, so CATCH_CONFIG_MAIN will cause linker errors!
#if defined(CATCH_CONFIG_MAIN)
#error CATCH_CONFIG_MAIN must not be defined: to run your own main(), use CATCH_CONFIG_RUNNER with #cmake:custom-main
#endif

#include <catch2/catch_all.hpp>
