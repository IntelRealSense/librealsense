// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>
#include "core/processing.h"
#include "proc/synthetic-stream.h"
#include "image.h"
#include "source.h"

namespace librealsense
{
	class LRS_EXTENSION_API reproject : public generic_processing_block
	{
	public:
		reproject(rs2_intrinsics intrinsics, rs2_extrinsics extrinsics);

	protected:
		reproject(rs2_intrinsics intrinsics, rs2_extrinsics extrinsics, const char* name);

		bool should_process(const rs2::frame& frame) override;
		rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

		std::shared_ptr<rs2::video_stream_profile> create_reproject_profile(const rs2::video_stream_profile& original_profile, rs2_intrinsics intrinsics, rs2_extrinsics extrinsics);

		rs2_intrinsics _intrinsics;
		rs2_extrinsics _extrinsics;
		float _depth_scale;
		std::shared_ptr<rs2::video_stream_profile> _reproject_profile;

	private:
		rs2::video_frame reproject::allocate_reproject_frame(const rs2::frame_source& source, const rs2::video_frame& frame);
		void reproject_frame(rs2::video_frame& to, const rs2::video_frame& from);


	};
}
