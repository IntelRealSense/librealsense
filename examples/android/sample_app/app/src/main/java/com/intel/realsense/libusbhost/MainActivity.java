package com.intel.realsense.libusbhost;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int CAMERA_PERMISSIONS = 1;
    private static final String[] VIDEO_PERMISSIONS = {
            Manifest.permission.CAMERA,
    };

    static {
        System.loadLibrary("usbhost");
    }

    final Handler mHandler = new Handler();
    private ByteBuffer depthBuffer;
    private ByteBuffer colorBuffer;

    private Button btnStart;
    private static final int DEPTH_HEIGHT = 720;
    private static final int DEPTH_WIDTH = 1280;
    private ColorConverter mDepthConverter;
    private RealsenseUsbHostManager mUsbHostManager;
    private boolean isStreaming = false;
    TextureView mTextureViewDepth;
    private ColorConverter mColorConferter;
    private TextureView mTextureViewColor;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mUsbHostManager = new RealsenseUsbHostManager(this);
        setContentView(R.layout.activity_main);
        depthBuffer = ByteBuffer.allocateDirect(DEPTH_HEIGHT * DEPTH_WIDTH * 2);
        depthBuffer.order(ByteOrder.nativeOrder());
        colorBuffer = ByteBuffer.allocateDirect(DEPTH_HEIGHT * DEPTH_WIDTH * 4);
        colorBuffer.order(ByteOrder.nativeOrder());
        mDepthConverter = new ColorConverter(this, ConversionType.DEPTH, DEPTH_WIDTH, DEPTH_HEIGHT);
        mColorConferter = new ColorConverter(this, ConversionType.RGBA, DEPTH_WIDTH, DEPTH_HEIGHT);
        mTextureViewDepth = findViewById(R.id.outputDepth);
        mTextureViewDepth.setSurfaceTextureListener(mDepthConverter);
        mTextureViewColor = findViewById(R.id.outputColor);
        mTextureViewColor.setSurfaceTextureListener(mColorConferter);
        btnStart = (Button) findViewById(R.id.btnStart);
        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isStreaming == true) {
                    btnStart.setText(R.string.stream_start);
                    stopRepeatingTask();
                    librsStopStreaming();
                    isStreaming = false;
                } else if (isStreaming == false) {
                    btnStart.setText(R.string.stream_stop);
                    startRepeatingTask();
                    librsStartStreaming(depthBuffer,colorBuffer,DEPTH_WIDTH,DEPTH_HEIGHT);
                    isStreaming = true;
                }
            }
        });
        getCameraPermission();
    }


    @Override
    public void onResume() {
        super.onResume();
        if(isStreaming){
            startRepeatingTask();
            librsStartStreaming(depthBuffer,colorBuffer,DEPTH_WIDTH,DEPTH_HEIGHT);
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        if(isStreaming)
            closeDevice();
    }

    Runnable updateBitmap = new Runnable() {
        @Override
        public void run() {
            try {

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mDepthConverter.process(depthBuffer);
                        mTextureViewDepth.invalidate();
                        mColorConferter.process(colorBuffer);
                        mTextureViewColor.invalidate();
                    }
                });
            } finally {
                // 100% guarantee that this always happens, even if
                // your update method throws an exception
                mHandler.postDelayed(updateBitmap, 1000 / 30);
            }
        }
    };

    private void getCameraPermission() {
        if (hasPermissionsGranted(VIDEO_PERMISSIONS))
            getUsbPermissions();
        else
            ActivityCompat.requestPermissions(this, VIDEO_PERMISSIONS, CAMERA_PERMISSIONS);

    }

    private boolean hasPermissionsGranted(String[] permissions) {
        for (String permission : permissions) {
            if (ActivityCompat.checkSelfPermission(this, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }
    private void getUsbPermissions() {
        if (mUsbHostManager.findDevice()) {
            btnStart.setText(R.string.stream_start);
            btnStart.setEnabled(true);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult");
        if (requestCode == CAMERA_PERMISSIONS) {
            if (grantResults.length == VIDEO_PERMISSIONS.length) {
                for (int result : grantResults) {
                    if (result != PackageManager.PERMISSION_GRANTED) {
                        Log.e(TAG,"Error getting permission!");
                        return;
                    }
                }
                getUsbPermissions();
            } else {
                Log.e(TAG,"Didnt get all permissions...");
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }//end onRequestPermissionsResult


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     *
     * @param l
     */
    public native boolean librsStartStreaming(ByteBuffer depthBuffer,ByteBuffer colorBuffer,int w,int h);

    public native boolean librsStopStreaming();

    void startRepeatingTask() {
        updateBitmap.run();
    }

    void stopRepeatingTask() {
        mHandler.removeCallbacks(updateBitmap);
    }

    private void closeDevice() {
        stopRepeatingTask();
        librsStopStreaming();
    }

}
