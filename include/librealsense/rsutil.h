#ifndef LIBREALSENSE_UTIL_INCLUDE_GUARD
#define LIBREALSENSE_UTIL_INCLUDE_GUARD

#include "rs.h"

/* Transform 3D coordinates to pixel coordinate in an image (which can potentially contain distortion) */
inline void rs_project_point_to_pixel(const float point[3], const rs_intrinsics & intrin, float pixel[2])
{
	float t[] = { point[0] / point[2], point[1] / point[2] }, r2 = t[0] * t[0] + t[1] * t[1];
	float f = 1 + r2 * (intrin.distortion_coeff[0] + r2 * (intrin.distortion_coeff[1] + r2 * intrin.distortion_coeff[4]));
	t[0] *= f; t[1] *= f;
	pixel[0] = intrin.principal_point[0] + intrin.focal_length[0] * (t[0] + 2*intrin.distortion_coeff[2]*t[0]*t[1] + intrin.distortion_coeff[3] * (r2 + 2*t[0]*t[0]));
	pixel[1] = intrin.principal_point[1] + intrin.focal_length[1] * (t[1] + 2*intrin.distortion_coeff[3]*t[0]*t[1] + intrin.distortion_coeff[2] * (r2 + 2*t[1]*t[1]));
}

/* Transform 3D coordinates to pixel coordinate in a rectified image (assumes no distortion) */
inline void rs_project_point_to_rectified_pixel(const float point[3], const rs_intrinsics & intrin, float pixel[2])
{
	pixel[0] = intrin.principal_point[0] + intrin.focal_length[0] * point[0] / point[2];
	pixel[1] = intrin.principal_point[1] + intrin.focal_length[1] * point[1] / point[2];
}

/* Transform pixel coordinates in a rectified image to 3D coordinates (assumes no distortion) */
inline void rs_deproject_rectified_pixel_to_point(const float pixel[2], float depth, const rs_intrinsics & intrin, float point[3])
{
	point[0] = depth * (pixel[0] - intrin.principal_point[0]) / intrin.focal_length[0];
	point[1] = depth * (pixel[1] - intrin.principal_point[1]) / intrin.focal_length[1];
	point[2] = depth;
}

/* Transform 3D coordinates from one viewpoint to 3D coordinates in another viewpoint */
inline void rs_transform_point_to_point(const float from_point[3], const rs_extrinsics & extrin, float to_point[3])
{
	to_point[0] = extrin.rotation[0] * from_point[0] + extrin.rotation[1] * from_point[1] + extrin.rotation[2] * from_point[2] + extrin.translation[0];
	to_point[1] = extrin.rotation[3] * from_point[0] + extrin.rotation[4] * from_point[1] + extrin.rotation[5] * from_point[2] + extrin.translation[1];
	to_point[2] = extrin.rotation[6] * from_point[0] + extrin.rotation[7] * from_point[1] + extrin.rotation[8] * from_point[2] + extrin.translation[2];
}

/* Transform from pixel coordinates from a rectified image to pixel coordinates in another image (assumes no distortion in the first image) */
inline void rs_transform_rectified_pixel_to_pixel(const float * from_pixel, float from_depth, const rs_intrinsics & from_intrin,
												  const rs_extrinsics & extrin, const rs_intrinsics & to_intrin, float * to_pixel)
{
	float from_point[3], to_point[3];
	rs_deproject_rectified_pixel_to_point(from_pixel, from_depth, from_intrin, from_point);
	rs_transform_point_to_point(from_point, extrin, to_point);
	rs_project_point_to_pixel(to_point, to_intrin, to_pixel);
}

/* Transform from pixel coordinates from a rectified image to pixel coordinates in another rectified image (assumes no distortion in either image) */
inline void rs_transform_rectified_pixel_to_rectified_pixel(const float * from_pixel, float from_depth, const rs_intrinsics & from_intrin,
															const rs_extrinsics & extrin, const rs_intrinsics & to_intrin, float * to_pixel)
{
	float from_point[3], to_point[3];
	rs_deproject_rectified_pixel_to_point(from_pixel, from_depth, from_intrin, from_point);
	rs_transform_point_to_point(from_point, extrin, to_point);
	rs_project_point_to_rectified_pixel(to_point, to_intrin, to_pixel);
}

#endif