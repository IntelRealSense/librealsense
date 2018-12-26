package com.intel.realsense.android;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.MediaCodec;
import android.media.MediaRecorder;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;

class DepthRecorder {

    private static final int REQUEST_VIDEO_PERMISSIONS = 1;
    private static final String FRAGMENT_DIALOG = "dialog";

    private static final String[] VIDEO_PERMISSIONS = {
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO,
    };

    private static final String TAG = DepthRecorder.class.getSimpleName();
    private final TextureView mTextureView;
    private final MediaRecorder mMediaRecorder;
    private final Activity mActivity;
    private String mNextVideoAbsolutePath;
    private boolean mIsRecordingVideo;
    private Surface mSurface;

    DepthRecorder(Activity activity,TextureView textureView){
        mActivity=activity;
        mTextureView=textureView;
        mMediaRecorder=new MediaRecorder();
    }

    private void setUpMediaRecorder() {

        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        if (mNextVideoAbsolutePath == null || mNextVideoAbsolutePath.isEmpty()) {
            mNextVideoAbsolutePath = getVideoFilePath(mActivity);
        }
        mMediaRecorder.setOutputFile(mNextVideoAbsolutePath);
        mMediaRecorder.setVideoEncodingBitRate(10000000);
        mMediaRecorder.setVideoFrameRate(30);
        mMediaRecorder.setVideoSize(mTextureView.getWidth(), mTextureView.getHeight());
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
        try {
            mMediaRecorder.prepare();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private String getVideoFilePath(Context context) {
        final File dir = context.getExternalFilesDir(null);
        return (dir == null ? "" : (dir.getAbsolutePath() + "/"))
                + System.currentTimeMillis() + ".mp4";
    }
    public Surface getSurface(){
        mSurface=MediaCodec.createPersistentInputSurface();
        mMediaRecorder.setInputSurface(mSurface);
        return mSurface;
    }
    public void startRecordingVideo() {
        if(mIsRecordingVideo)
            return;
            setUpMediaRecorder();
            SurfaceTexture texture = mTextureView.getSurfaceTexture();
            assert texture != null;
            texture.setDefaultBufferSize(mTextureView.getWidth(), mTextureView.getHeight());

            // Start a capture session
            // Once the session starts, we can update the UI and start recording
                            mIsRecordingVideo = true;

                            // Start recording
                            mMediaRecorder.start();


    }


    public void stopRecordingVideo() {
        if(!mIsRecordingVideo)
            return;
        // UI
        mIsRecordingVideo = false;
        // Stop recording
        mMediaRecorder.stop();
        mMediaRecorder.reset();

        Activity activity = mActivity;
        if (null != activity) {
            Toast.makeText(activity, "Video saved: " + mNextVideoAbsolutePath,
                    Toast.LENGTH_SHORT).show();
            Log.d(TAG, "Video saved: " + mNextVideoAbsolutePath);
        }
        mNextVideoAbsolutePath = null;
    }

}
