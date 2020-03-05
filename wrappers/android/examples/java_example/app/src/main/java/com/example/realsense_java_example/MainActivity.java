package com.example.realsense_java_example;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;

import com.intel.realsense.librealsense.DepthFrame;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;

import java.text.DecimalFormat;

public class MainActivity extends AppCompatActivity {
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;

    private RsContext mRsContext;

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(!mStreamingThread.isAlive())
            mStreamingThread.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mStreamingThread.interrupt();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //RsContext.init must be called once in the application's lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(getApplicationContext());

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(new DeviceListener() {
            @Override
            public void onDeviceAttach() {
                mStreamingThread.start();
            }

            @Override
            public void onDeviceDetach() {
                mStreamingThread.interrupt();
            }
        });

        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }
    }

    private Thread mStreamingThread = new Thread(new Runnable() {
        @Override
        public void run() {
            try {
                stream();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    });

    //Start streaming and print the distance of the center pixel in the depth frame.
    private void stream() throws Exception {
        Pipeline pipe = new Pipeline();
        // try statement needed here to release resources allocated by the Pipeline:start() method
        try(PipelineProfile pp = pipe.start()){}
        final DecimalFormat df = new DecimalFormat("#.##");

        while (!mStreamingThread.isInterrupted())
        {
            try (FrameSet frames = pipe.waitForFrames()) {
                try (Frame f = frames.first(StreamType.DEPTH))
                {
                    DepthFrame depth = f.as(Extension.DEPTH_FRAME);
                    final float deptValue = depth.getDistance(depth.getWidth()/2, depth.getHeight()/2);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            TextView textView = findViewById(R.id.distanceTextView);
                            textView.setText("Distance: " + df.format(deptValue));
                        }
                    });
                }
            }
        }
        pipe.stop();
    }
}
