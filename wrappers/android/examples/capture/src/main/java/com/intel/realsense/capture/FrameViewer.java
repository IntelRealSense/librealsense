package com.intel.realsense.capture;

import android.app.Activity;
import android.graphics.Bitmap;
import android.widget.ImageView;

import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.VideoFrame;

import java.nio.ByteBuffer;

public class FrameViewer {
    private final ImageView mImageView;
    private Bitmap mBitmap;
    private ByteBuffer mBuffer;

    public FrameViewer(ImageView imageView){
        mImageView = imageView;
    }

    private Bitmap.Config getBitmapFormat(Frame f){
        StreamFormat format = f.getProfile().getFormat();
        switch (format){
            case Z16: return Bitmap.Config.RGB_565;
            case RGBA8: return Bitmap.Config.ARGB_8888;
            case Y8: return Bitmap.Config.ALPHA_8;
            default: return null;
        }
    }

    public void show(Activity activity, VideoFrame f){
        if(f == null)
            return;

        if(mBitmap == null){
            mBitmap = Bitmap.createBitmap(f.getWidth(), f.getHeight(), getBitmapFormat(f));
            mBuffer = ByteBuffer.allocateDirect(mBitmap.getByteCount());
        }
        f.getData(mBuffer.array());
        mBuffer.rewind();
        mBitmap.copyPixelsFromBuffer(mBuffer);
        activity.runOnUiThread(new Runnable(){
            public void run() {
                mImageView.setImageBitmap(mBitmap);
            }
        });
    }
}
