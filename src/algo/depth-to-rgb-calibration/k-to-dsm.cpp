//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "k-to-dsm.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


rs2_intrinsics_double rotate_k_mat(rs2_intrinsics_double k_mat)
{
	rs2_intrinsics_double res = k_mat;
	res.ppx = k_mat.width - 1 - k_mat.ppx;
	res.ppy = k_mat.height - 1 - k_mat.ppy;

	return res;
}
rs2_dsm_params librealsense::algo::depth_to_rgb_calibration::convert_new_k_to_DSM(rs2_intrinsics_double old_k, rs2_intrinsics_double new_k)
{
	auto old_k_raw = rotate_k_mat(old_k);
	auto new_k_raw = rotate_k_mat(new_k);

	return rs2_dsm_params();
}
