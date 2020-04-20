// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <opencv2/opencv.hpp>   // Include OpenCV API

void downsample_min_4x4(const cv::Mat& source, cv::Mat* pDest);