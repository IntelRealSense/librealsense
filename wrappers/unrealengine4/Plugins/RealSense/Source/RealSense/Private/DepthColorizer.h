#pragma once
#include "RealSenseNative.h"

namespace rs2_utils {

struct depth_pixel { uint8_t r, g, b, a; };

void colorize_depth(depth_pixel* dst, const rs2::video_frame& depth, int colormap_id, float depth_min, float depth_max, float depth_units, bool equalize);

};
