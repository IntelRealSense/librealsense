package com.intel.realsense.capture;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Extrinsic;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Intrinsic;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.Pixel;
import com.intel.realsense.librealsense.Point_3D;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Utils;
import com.intel.realsense.librealsense.VideoFrame;

// Aim of this fragment is to show how the following functions must be used:
// From class Utils:
// deprojectPixelToPoint
// transformPointToPoint
// projectPointToPixel
// getFov
// project2dPixelToDepthPixel


private int framesCounter = 1;
        Runnable mStreaming = new Runnable() {
@Override
public void run() {
        try {
            try(FrameSet frames = mPipeline.waitForFrames()) {
                try(FrameSet processed = frames.applyFilter(mColorizer)) {
                    mGLSurfaceView.upload(processed);
                    if((++framesCounter % 80) == 0) {
                        // getting depth frame
                        DepthFrame depthFrame = frames.first(StreamType.DEPTH).as(Extension.DEPTH_FRAME);
                        Intrinsic depthFrameIntrinsic = depthFrame.getProfile().getIntrinsic();
                        // getting depth frame field of view
                        float fov[] = Utils.getFov(depthFrameIntrinsic);
                        Log.d(TAG, "fov = (" + fov[0] + ", " + fov[1] + ")");
                        // getting color frame
                        VideoFrame colorFrame = frames.first(StreamType.COLOR).as(Extension.VIDEO_FRAME);
                        Intrinsic colorFrameIntrinsic = colorFrame.getProfile().getIntrinsic();
                        // computing extrinsics between the profiles
                        Extrinsic depthToColorExtrinsic = depthFrame.getProfile().getExtrinsicTo(colorFrame.getProfile());
                        Extrinsic colorToDepthExtrinsic = colorFrame.getProfile().getExtrinsicTo(depthFrame.getProfile());
                        // finding depth pixel with distance not zero (means that there is depth data in this pixel)
                        int w = depthFrameIntrinsic.getWidth();
                        int h = depthFrameIntrinsic.getHeight();
                        byte[] depthFrameData = new byte[ w * h ];
                        depthFrame.getData(depthFrameData);
                        float depthDataAtMiddleOfFrame = depthFrameData[w * h / 2];
                        int halfWidth = w / 2;
                        while (depthDataAtMiddleOfFrame < 0.2) {
                            halfWidth += 10;
                            depthDataAtMiddleOfFrame = depthFrameData[halfWidth * h];
                        }
                        // using get distance api to convert from raw distance to meters
                        float depthAtMiddleOfFrame = depthFrame.getDistance(halfWidth, h / 2);
                        Log.d(TAG, "depth = " + depthAtMiddleOfFrame);
                        // deproject, transform, project - in order to convert from depth pixel to color pixel
                        Pixel depth_pixel = new Pixel(halfWidth, h/2);
                        Point_3D depth_point = Utils.deprojectPixelToPoint(depthFrameIntrinsic, depth_pixel, depthAtMiddleOfFrame );
                        Point_3D color_point = Utils.transformPointToPoint(depthToColorExtrinsic, depth_point );
                        Pixel color_pixel = Utils.projectPointToPixel(colorFrameIntrinsic, color_point);
                        Log.d(TAG, "depthPoint = (" + depth_point.mX + ", " + depth_point.mY + ", " + depth_point.mZ + ")");
                        Log.d(TAG, "colorPoint = (" + color_point.mX + ", " + color_point.mY + ", " + color_point.mZ + ")");
                        Log.d(TAG, "pixel = (" + depth_pixel.mX + ", " + depth_pixel.mY + ")");
                        Log.d(TAG, "color pixel = (" + color_pixel.mX + ", " + color_pixel.mY + ")");
                        // convert color pixel to depth pixel
                        float depthMin = 0.1f;
                        float depthMax = 10.f;
                        Pixel toPixel = Utils.project2dPixelToDepthPixel(depthFrame, mDepthScale, depthMin, depthMax,
                        depthFrameIntrinsic, colorFrameIntrinsic, colorToDepthExtrinsic, depthToColorExtrinsic, color_pixel);
                        Log.d(TAG, "to_pixel = (" + toPixel.mX + ", " + toPixel.mY + ")");
                    }
                }
            }
            mHandler.post(mStreaming);
        }
        catch (Exception e) {
            Log.e(TAG, "streaming, error: " + e.getMessage());
        }
    }
};
