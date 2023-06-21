package com.intel.realsense.capture;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.intel.realsense.librealsense.Align;
import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DepthFrame;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameReleaser;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

import org.opencv.android.OpenCVLoader;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.Date;

public class MainActivity extends AppCompatActivity {

    static {
        if(!OpenCVLoader.initDebug())
        {
            Log.d("opencv","初始化失败");
        }
    }

    private static final String TAG = "librs capture example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;
    private static final int PERMISSIONS_REQUEST_WRITE = 1;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceView;
    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private Pipeline mPipeline;
    private Colorizer mColorizer;
    private RsContext mRsContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
        mBackGroundText = findViewById(R.id.connectCameraText);
        mGLSurfaceView = findViewById(R.id.glSurfaceView);

        Button button1 = (Button) findViewById(R.id.startRecordFab);
        button1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleRecording();
            }
        });

        mGLSurfaceView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);


        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
            return;
        }

        mPermissionsGranted = true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLSurfaceView.close();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_WRITE);
            return;
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
            return;
        }

        mPermissionsGranted = true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(mPermissionsGranted)
            init();
        else
            Log.e(TAG, "missing permissions");
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(mRsContext != null)
            mRsContext.close();
        stop();
        mColorizer.close();
        mPipeline.close();
    }

    private void init(){
        //RsContext.init must be called once in the application lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(mAppContext);

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(mListener);

        mPipeline = new Pipeline();
        mColorizer = new Colorizer();

        try(DeviceList dl = mRsContext.queryDevices()){
            if(dl.getDeviceCount() > 0) {
                showConnectLabel(false);
                start();
            }
        }
    }

    private void showConnectLabel(final boolean state){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBackGroundText.setVisibility(state ? View.VISIBLE : View.GONE);
            }
        });
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            showConnectLabel(false);
        }

        @Override
        public void onDeviceDetach() {
            showConnectLabel(true);
            stop();
        }
    };

    Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameReleaser fr = new FrameReleaser()) {
                    FrameSet framesProcessed = mPipeline.waitForFrames().releaseWith(fr);
                    mGLSurfaceView.upload(framesProcessed);
                }
                mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    private void configAndStart() throws Exception {
        try(Config config  = new Config())
        {
            config.enableStream(StreamType.DEPTH, 640, 480);
            config.enableStream(StreamType.COLOR, 640, 480);
            // try statement needed here to release resources allocated by the Pipeline:start() method
            try(PipelineProfile pp = mPipeline.start(config)){}
        }
    }

    private synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            Log.d(TAG, "try start streaming");
            mGLSurfaceView.clear();
            configAndStart();
            mIsStreaming = true;
            mHandler.post(mStreaming);
            Log.d(TAG, "streaming started successfully");
        } catch (Exception e) {
            Log.d(TAG, "failed to start streaming");
        }
    }

    private synchronized void stop() {
        if(!mIsStreaming)
            return;
        try {
            Log.d(TAG, "try stop streaming");
            mIsStreaming = false;
            mHandler.removeCallbacks(mStreaming);
            mPipeline.stop();
            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
        }
    }

    public void toggleRecording() {
        try (FrameReleaser fr = new FrameReleaser()) {
            try (FrameSet frames = mPipeline.waitForFrames().releaseWith(fr)) {
//                        Frame depth = frames.first(StreamType.DEPTH).releaseWith(fr);
//                        DepthFrame depthFrame = depth.as(Extension.DEPTH_FRAME);
//                        depthFrame.releaseWith(fr);
                // saving raw depth data
//                        Mat mDepth = new Mat(depthFrame.getHeight(), depthFrame.getWidth(), CvType.CV_16UC1);
//                        int size = (int) (mDepth.total() * mDepth.elemSize());
//                        byte[] return_buff_depth = new byte[size];
//                        depthFrame.getData(return_buff_depth);
//                        short[] shorts = new short[size / 2];
//                        ByteBuffer.wrap(return_buff_depth).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
//                        mDepth.put(0, 0, shorts);
                Align align = new Align(StreamType.COLOR);
                String folderPath = "/storage/emulated/0/graduateDesign/";
                String profile = "lhh";
                SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss");
                String currentDateAndTime = sdf.format(new Date());

                //Path path = Paths.get(folderPath, String.format("%spixelDepthData%s.npy", profile, currentDateAndTime));
                //NpyFile.write(path, shorts);
                // applying Colorizer
                try (FrameSet processed = frames.applyFilter(align).releaseWith(fr).applyFilter(mColorizer).releaseWith(fr)) {
                    Frame depthColored = processed.first(StreamType.DEPTH).releaseWith(fr);
                    DepthFrame depthColorFrame = depthColored.as(Extension.DEPTH_FRAME);
                    depthColorFrame.releaseWith(fr);
                    Frame color = processed.first(StreamType.COLOR).releaseWith(fr);
                    VideoFrame colorFrame = color.as(Extension.VIDEO_FRAME);
                    colorFrame.releaseWith(fr);

                    // capturing color image
                    Mat mRGB = new Mat(colorFrame.getHeight(), colorFrame.getWidth(), CvType.CV_8UC3);
                    byte[] return_buff = new byte[colorFrame.getDataSize()];
                    colorFrame.getData(return_buff);
                    mRGB.put(0, 0, return_buff);

                    Bitmap bmp = Bitmap.createBitmap(mRGB.cols(), mRGB.rows(), Bitmap.Config.ARGB_8888);
                    org.opencv.android.Utils.matToBitmap(mRGB, bmp);

                    File imgFile = new File(folderPath, profile + "colorFrame-" + currentDateAndTime + ".jpg");
                    try {
                        FileOutputStream out2 = new FileOutputStream(imgFile);
                        bmp.compress(Bitmap.CompressFormat.JPEG, 100, out2);
                        out2.flush();
                        out2.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    // capturing colorized depth image
                    Mat mDepthColorized = new Mat(depthColorFrame.getHeight(), depthColorFrame.getWidth(), CvType.CV_8UC3);
                    byte[] return_buff_DepthColor = new byte[depthColorFrame.getDataSize()];
                    depthColorFrame.getData(return_buff_DepthColor);
                    mDepthColorized.put(0, 0, return_buff_DepthColor);

                    Bitmap bmpDepthColor = Bitmap.createBitmap(mDepthColorized.cols(), mDepthColorized.rows(), Bitmap.Config.ARGB_8888);
                    org.opencv.android.Utils.matToBitmap(mDepthColorized, bmpDepthColor);

                    File imgFileDepthColor = new File(folderPath, profile + "depthFrame-" + currentDateAndTime + ".jpg");
                    try {
                        FileOutputStream out3 = new FileOutputStream(imgFileDepthColor);
                        bmpDepthColor.compress(Bitmap.CompressFormat.JPEG, 100, out3);
                        out3.flush();
                        out3.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}

