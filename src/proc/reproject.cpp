// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "../include/librealsense2/rsutil.h"

#include "core/video.h"
#include "proc/synthetic-stream.h"
#include "environment.h"
#include "reproject.h"
#include "stream.h"




namespace librealsense
{

	reproject::reproject(rs2_intrinsics intrinsics, rs2_extrinsics extrinsics, const char* name)
		: generic_processing_block(name),
		_intrinsics(intrinsics),
		_extrinsics(extrinsics),
		_depth_scale(0.001)
	{}
	reproject::reproject(rs2_intrinsics intrinsics, rs2_extrinsics extrinsics) :
		reproject(intrinsics, extrinsics, "Reproject") {}


	template<int N> struct bytes { byte b[N]; };
	template<class GET_DEPTH, class TRANSFER_PIXEL>
	void align_images(const rs2_intrinsics& depth_intrin, const rs2_extrinsics& depth_to_other,
		const rs2_intrinsics& other_intrin, GET_DEPTH get_depth, TRANSFER_PIXEL transfer_pixel)
	{
		// Iterate over the pixels of the depth image
#pragma omp parallel for schedule(dynamic)
		for (int depth_y = 0; depth_y < depth_intrin.height; ++depth_y)
		{
			int depth_pixel_index = depth_y * depth_intrin.width;
			for (int depth_x = 0; depth_x < depth_intrin.width; ++depth_x, ++depth_pixel_index)
			{
				// Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
				if (float depth = get_depth(depth_pixel_index))
				{
					// Map the top-left corner of the depth pixel onto the other image
					float depth_pixel[2] = { depth_x - 0.5f, depth_y - 0.5f }, depth_point[3], other_point[3], other_pixel[2];
					rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
					rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
					rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
					const int other_x0 = static_cast<int>(other_pixel[0] + 0.5f);
					const int other_y0 = static_cast<int>(other_pixel[1] + 0.5f);

					// Map the bottom-right corner of the depth pixel onto the other image
					depth_pixel[0] = depth_x + 0.5f; depth_pixel[1] = depth_y + 0.5f;
					rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
					rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
					rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
					const int other_x1 = static_cast<int>(other_pixel[0] + 0.5f);
					const int other_y1 = static_cast<int>(other_pixel[1] + 0.5f);

					if (other_x0 < 0 || other_y0 < 0 || other_x1 >= other_intrin.width || other_y1 >= other_intrin.height)
						continue;

					// Transfer between the depth pixels and the pixels inside the rectangle on the other image
					for (int y = other_y0; y <= other_y1; ++y)
					{
						for (int x = other_x0; x <= other_x1; ++x)
						{
							transfer_pixel(depth_pixel_index, y * other_intrin.width + x);
						}
					}
				}
			}
		}
	}


	bool reproject::should_process(const rs2::frame& frame)
	{
		if (!frame || frame.is<rs2::frameset>())
			return false;

		if (frame.get_profile().stream_type() != RS2_STREAM_DEPTH)
			return false;

		return true;
	}

	rs2::frame reproject::process_frame(const rs2::frame_source& source, const rs2::frame& f)
	{
		rs2::frame rv;
		auto depth = f.as<rs2::depth_frame>();
		_depth_scale = ((librealsense::depth_frame*)depth.get())->get_units();
		auto aligned_frame = allocate_reproject_frame(source, depth);
		reproject_frame(aligned_frame, depth);
		return aligned_frame;
	}

	std::shared_ptr<rs2::video_stream_profile> reproject::create_reproject_profile(const rs2::video_stream_profile& original_profile, rs2_intrinsics intrinsics, rs2_extrinsics extrinsics)
	{
		if (_reproject_profile)
		{
			return _reproject_profile;
		}
		auto reproject_profile = std::make_shared<rs2::video_stream_profile>(original_profile);

		auto profile = As<video_stream_profile_interface>(reproject_profile->get()->profile);

		profile->set_intrinsics([intrinsics]() { return intrinsics; });
		reproject_profile->register_extrinsics_to(original_profile, extrinsics);
		_reproject_profile = reproject_profile;
		return _reproject_profile;
	}

	rs2::video_frame reproject::allocate_reproject_frame(const rs2::frame_source& source, const rs2::video_frame& frame)
	{
		rs2::frame rv;
		auto frame_bytes_per_pixel = frame.get_bytes_per_pixel();
		auto frame_profile = frame.get_profile().as<rs2::video_stream_profile>();
		
		auto to_profile = create_reproject_profile(frame_profile, _intrinsics, _extrinsics);
		rv = source.allocate_video_frame(
			*_reproject_profile,
			frame,
			frame_bytes_per_pixel,
			to_profile->width(),
			to_profile->height(),
			to_profile->width() * frame_bytes_per_pixel,
			RS2_EXTENSION_DEPTH_FRAME);
		return rv;
	}
	void reproject::reproject_frame(rs2::video_frame& reprojected_frame, const rs2::video_frame& from)
	{
		byte* reproject_data = reinterpret_cast<byte*>(const_cast<void*>(reprojected_frame.get_data()));
		auto reproject_profile = reprojected_frame.get_profile().as<rs2::video_stream_profile>();
		memset(reproject_data, 0,
			reproject_profile.height() *
			reproject_profile.width() *
			reprojected_frame.get_bytes_per_pixel()
		);

		auto depth_profile = from.get_profile().as<rs2::video_stream_profile>();

		auto z_intrin = depth_profile.get_intrinsics();
		auto other_intrin = reproject_profile.get_intrinsics();
		auto z_to_other = _extrinsics;

		auto z_pixels = reinterpret_cast<const uint16_t*>(from.get_data());
		auto out_z = (uint16_t*)(reproject_data);
		float z_scale = _depth_scale;
		align_images(z_intrin, z_to_other, other_intrin,
			[z_pixels, z_scale](int z_pixel_index) { return z_scale * z_pixels[z_pixel_index]; },
			[out_z, z_pixels](int z_pixel_index, int other_pixel_index)
			{
				out_z[other_pixel_index] = out_z[other_pixel_index] ?
					std::min((int)out_z[other_pixel_index], (int)z_pixels[z_pixel_index]) :
					z_pixels[z_pixel_index];
			});
	}
}
