// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once


namespace rsutils {
namespace concurrency {


// A handler for catching Control-C input from the user
// 
// NOTE: only a single instance should be in use: using more than one has undefined behavior
//
class control_c_handler
{
public:
    control_c_handler();
    ~control_c_handler();

    // Return true if a Control-C happened
    bool was_pressed() const;

    // Make was_pressed() return false
    void reset();

    // Block until a Control-C happens
    // If already was_pressed(), the state is cleared and a new Control-C is waited on
    void wait();
};


}  // namespace concurrency
}  // namespace rsutils
