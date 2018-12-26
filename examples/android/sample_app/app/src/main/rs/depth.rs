#pragma version(1)
#pragma rs java_package_name(com.intel.realsense.android)
#pragma rs_fp_relaxed

rs_allocation aHistogramColorMap;
int gMaxValue = 3000;

/**
 * 16bit lumiance to RGBA
 * using the gHistogramColorMap Global allocation
 */
uchar4 __attribute__((kernel)) zImageToDepthHistogram(const uint16_t in, uint32_t x, uint32_t y){
            uchar4 black;
            black.r=0;
            black.g=0;
            black.b=0;
            black.a=0;

            if(in>gMaxValue)
                return black;
            else
			    return rsGetElementAt_uchar4(aHistogramColorMap,in);
}