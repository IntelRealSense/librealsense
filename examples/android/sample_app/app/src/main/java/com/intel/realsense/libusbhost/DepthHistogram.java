package com.intel.realsense.libusbhost;

import android.graphics.Color;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.Type;

public class DepthHistogram {
    private final Allocation allocation;
    private int[] mHistogramColorMap;
    ScriptC_depth mScriptDepth;

    DepthHistogram(int max_distance, RenderScript mRS) {
        mScriptDepth=new ScriptC_depth(mRS);
        allocation = Allocation.createTyped(mRS,
                Type.createX(mRS,
                        Element.RGBA_8888(mRS), max_distance));
        createHistogramColorMap(max_distance);
        mScriptDepth.set_aHistogramColorMap(allocation);

    }

    public void DepthToRGBA(Allocation in, Allocation out){
        mScriptDepth.forEach_zImageToDepthHistogram(in,out);
    }
    /**
     * this function creates a histogram of the distance
     *
     * @param max the maximum distance the histogram should consider
     */
    private void createHistogramColorMap(int max) {
        mHistogramColorMap = new int[max];

        final float HISTOGRAM_VALUE = 0.9f, HISTOGRAM_SATURATION = 0.86f;
        float[] hsv = new float[3];
        hsv[1] = HISTOGRAM_SATURATION;
        hsv[2] = HISTOGRAM_VALUE;
        float ratio=1.0f/(float)max;
        mHistogramColorMap[0] = Color.BLACK;
        for (int i = 1; i < max; i++) {
            hsv[0] = 360.0f * (i * ratio);
            mHistogramColorMap[i] = Color.HSVToColor(hsv);
        }
        allocation.copyFromUnchecked(mHistogramColorMap);
    }
}
