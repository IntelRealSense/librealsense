#include "DepthColorizer.h"
#include "PCH.h"
#include "ColorMap.inl"

namespace rs2_utils {

void make_equalized_histogram(depth_pixel* dst, const rs2::video_frame& depth, const color_map& cm)
{
	SCOPED_PROFILER;

	const size_t max_depth = 0x10000;
	static size_t histogram[max_depth];
	memset(histogram, 0, sizeof(histogram));

	const auto w = depth.get_width(), h = depth.get_height();
	const auto depth_data = (const uint16_t*)depth.get_data();

	for (auto i = 0; i < w*h; ++i) ++histogram[depth_data[i]];
	for (auto i = 2; i < max_depth; ++i) histogram[i] += histogram[i - 1];

	const auto scale = 1.0f / (float)histogram[0xFFFF];

	for (auto i = 0; i < w*h; ++i)
	{
		const auto d = depth_data[i];
		if (d)
		{
			//auto f = histogram[d] / (float)histogram[0xFFFF];
			const auto f = histogram[d] * scale;
			const auto c = cm.get(f);

			dst->r = (uint8_t)c.x;
			dst->g = (uint8_t)c.y;
			dst->b = (uint8_t)c.z;
			dst->a = 255;
		}
		else
		{
			dst->r = 0;
			dst->g = 0;
			dst->b = 0;
			dst->a = 255;
		}
		dst++;
	}
}

void make_value_cropped_frame(depth_pixel* dst, const rs2::video_frame& depth, const color_map& cm, float depth_min, float depth_max, float depth_units)
{
	SCOPED_PROFILER;

	const auto scale = 1.0f / (depth_max - depth_min);
	const auto w = depth.get_width(), h = depth.get_height();
	const auto depth_data = (const uint16_t*)depth.get_data();

	for (auto i = 0; i < w*h; ++i)
	{
		const auto d = depth_data[i];
		if (d)
		{
			//const auto f = (d * depth_units - depth_min) / (depth_max - depth_min);
			const auto f = (d * depth_units - depth_min) * scale;
			const auto c = cm.get(f);

			dst->r = (uint8_t)c.x;
			dst->g = (uint8_t)c.y;
			dst->b = (uint8_t)c.z;
			dst->a = 255;
		}
		else
		{
			dst->r = 0;
			dst->g = 0;
			dst->b = 0;
			dst->a = 255;
		}
		dst++;
	}
}

void colorize_depth(depth_pixel* dst, const rs2::video_frame& depth, int colormap_id, float depth_min, float depth_max, float depth_units, bool equalize)
{
	auto& cm = *colormap_presets[colormap_id];

	if (equalize) make_equalized_histogram(dst, depth, cm);
	else make_value_cropped_frame(dst, depth, cm, depth_min, depth_max, depth_units);
}

} // namespace
