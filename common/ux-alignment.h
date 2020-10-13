// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

//This function checks whether there is UI misalignment,
//sometimes caused by outdated graphics card drivers,
//it puts a red pixel (1x1 rect) on the top left corner of the window,
//take a snapshot and check the value of the top left corner pixel
//if its red - there is no misalignment
//otherwise - there is misalignment

bool is_gui_aligned(GLFWwindow *win);
