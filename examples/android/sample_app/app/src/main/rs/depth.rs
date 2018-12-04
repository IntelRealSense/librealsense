#pragma version(1)
#pragma rs java_package_name(com.intel.realsense.libusbhost)
#pragma rs_fp_relaxed

rs_allocation aHistogramColorMap;

/**
 * 16bit lumiance to RGBA
 * using the gHistogramColorMap Global allocation
 */
uchar4 __attribute__((kernel)) zImageToDepthHistogram(const uint16_t in, uint32_t x, uint32_t y){
			return rsGetElementAt_uchar4(aHistogramColorMap,in);
}