package com.intel.realsense.recording;

import static org.opencv.core.CvType.CV_16U;
import static org.opencv.core.CvType.CV_16UC1;
import static org.opencv.core.CvType.CV_8UC1;
import static org.opencv.core.CvType.CV_8UC3;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.media.Image;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;

import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
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
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgcodecs.Imgcodecs;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ShortBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    //Opencv库初始化
    static {
        if(!OpenCVLoader.initDebug())
        {
            Log.d("opencv","初始化失败");
        }
    }

    private static final String TAG = "librs recording example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;
    private static final int PERMISSIONS_REQUEST_WRITE = 1;
    //深度单位比例
    private static final double DEPTH_SCALE = 0.0010000000474974513;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceView;
    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private Pipeline mPipeline;
    private RsContext mRsContext;
    //新建着色器
    private Colorizer mColorizer;

    private FloatingActionButton mStartRecordFab;
    private FloatingActionButton mStopRecordFab;
    //新建拍照按钮
    public Button button1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
        mBackGroundText = findViewById(R.id.connectCameraText);
        mGLSurfaceView = findViewById(R.id.glSurfaceView);

        mStartRecordFab = findViewById(R.id.startRecordFab);
        mStopRecordFab = findViewById(R.id.stopRecordFab);

        //拍照按钮调用
        button1 = findViewById(R.id.Record);

        mStartRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleRecording();
            }
        });
        mStopRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleRecording();
            }
        });

        //调用拍照函数
        button1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Recording();
            }
        });

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
        //更新资源
        mColorizer.close();
        mPipeline.close();
    }

    private String getFilePath(){
        File folder = new File(getExternalFilesDir(null).getAbsolutePath() + File.separator + "rs_bags");
        folder.mkdir();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss");
        String currentDateAndTime = sdf.format(new Date());
        File file = new File(folder, currentDateAndTime + ".bag");
        return file.getAbsolutePath();
    }

    void init(){
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
                start(false);
            }
        }
    }

    private void showConnectLabel(final boolean state){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBackGroundText.setVisibility(state ? View.VISIBLE : View.GONE);
                mStartRecordFab.setVisibility(!state ? View.VISIBLE : View.GONE);
                mStopRecordFab.setVisibility(View.GONE);
            }
        });
    }

    private void toggleRecording(){
        stop();
        start(mStartRecordFab.getVisibility() == View.VISIBLE);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mStartRecordFab.setVisibility(mStartRecordFab.getVisibility() == View.GONE ? View.VISIBLE : View.GONE);
                mStopRecordFab.setVisibility(mStopRecordFab.getVisibility() == View.GONE ? View.VISIBLE : View.GONE);
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

    //初始化函数
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

    private synchronized void start(boolean record) {
        if(mIsStreaming)
            return;
        try{
            mGLSurfaceView.clear();
            Log.d(TAG, "try start streaming");
            try(Config cfg = new Config()) {
                //设置分辨率
                cfg.enableStream(StreamType.DEPTH, 1280, 720, StreamFormat.Z16);
                cfg.enableStream(StreamType.COLOR, 1280, 720, StreamFormat.RGB8);
                if (record)
                    cfg.enableRecordToFile(getFilePath());
                // try statement needed here to release resources allocated by the Pipeline:start() method
                try(PipelineProfile pp = mPipeline.start(cfg)){}
            }
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
        }  catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
            mPipeline = null;
        }
    }

    //拍照函数
    public void Recording() {try (FrameReleaser fr = new FrameReleaser()) {
        try (FrameSet frames = mPipeline.waitForFrames().releaseWith(fr)) {
            //设置对齐函数，将深度图对齐到彩色图
            Align align = new Align(StreamType.COLOR);
            //保存路径设置
            String folderPath = "/storage/emulated/0/graduateDesign/";
            String profile = "lhh";
            SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss");
            String currentDateAndTime = sdf.format(new Date());

            // applying Colorizer 创建深度,彩色,灰度帧
            try (FrameSet processed = frames.applyFilter(align).releaseWith(fr).applyFilter(mColorizer).releaseWith(fr)) {

                Frame depthColored = processed.first(StreamType.DEPTH,StreamFormat.RGB8).releaseWith(fr);
                DepthFrame depthColorFrame = depthColored.as(Extension.DEPTH_FRAME);
                depthColorFrame.releaseWith(fr);

                Frame color = processed.first(StreamType.COLOR, StreamFormat.RGB8).releaseWith(fr);
                VideoFrame colorFrame = color.as(Extension.VIDEO_FRAME);
                colorFrame.releaseWith(fr);

                Frame depth = processed.first(StreamType.DEPTH,StreamFormat.Z16).releaseWith(fr);
                DepthFrame depthFrame = depth.as(Extension.DEPTH_FRAME);
                depthFrame.releaseWith(fr);

                // capturing color image 保存彩色图
                Mat mRGB = new Mat(colorFrame.getHeight(), colorFrame.getWidth(), CvType.CV_8UC3);
                byte[] return_buff = new byte[colorFrame.getDataSize()];
                colorFrame.getData(return_buff);
                mRGB.put(0, 0, return_buff);

                Bitmap bmp = Bitmap.createBitmap(mRGB.cols(), mRGB.rows(), Bitmap.Config.ARGB_8888);
                org.opencv.android.Utils.matToBitmap(mRGB, bmp);
                Bitmap newbmp = rotateBitmapByDegree(bmp, 90); //旋转操作

                File imgFile = new File(folderPath, profile + "colorFrame-" + currentDateAndTime + ".png");
                try {
                    FileOutputStream out2 = new FileOutputStream(imgFile);
                    newbmp.compress(Bitmap.CompressFormat.PNG, 100, out2);
                    out2.flush();
                    out2.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                // capturing colorized depth image 保存伪彩深度图
                Mat mDepthColorized = new Mat(depthColorFrame.getHeight(), depthColorFrame.getWidth(), CvType.CV_8UC3);
                byte[] return_buff_DepthColor = new byte[depthColorFrame.getDataSize()];
                depthColorFrame.getData(return_buff_DepthColor);
                mDepthColorized.put(0, 0, return_buff_DepthColor);

                Bitmap bmpDepthColor = Bitmap.createBitmap(mDepthColorized.cols(), mDepthColorized.rows(), Bitmap.Config.ARGB_8888);
                org.opencv.android.Utils.matToBitmap(mDepthColorized, bmpDepthColor);
                Bitmap newbmpDepthColor = rotateBitmapByDegree(bmpDepthColor, 90); //旋转操作

                File imgFileDepthColor = new File(folderPath, profile + "depthFrame-" + currentDateAndTime + ".png");
                try {
                    FileOutputStream out3 = new FileOutputStream(imgFileDepthColor);
                    newbmpDepthColor.compress(Bitmap.CompressFormat.PNG, 100, out3);
                    out3.flush();
                    out3.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                //保存灰度深度图
                Mat mDepth = new Mat(depthFrame.getHeight(), depthFrame.getWidth(), CV_16UC1);
                int size = (int) (mDepth.total() * mDepth.elemSize());
                byte [] return_buff_Depth = new byte[size];
                depthFrame.getData(return_buff_Depth);
                short [] shorts = new short[size/2];
                ByteBuffer.wrap(return_buff_Depth).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
                mDepth.put(0, 0, shorts);
                Mat mDepth8UC1 = new Mat(mDepth.rows(), mDepth.cols(), CV_8UC1);
                mDepth.convertTo(mDepth8UC1, CV_8UC1, 1/255.0);

                Bitmap bmpDepth = Bitmap.createBitmap(mDepth8UC1.cols(), mDepth8UC1.rows(), Bitmap.Config.ARGB_8888);
                org.opencv.android.Utils.matToBitmap(mDepth8UC1, bmpDepth);
                Bitmap newbmpDepth = rotateBitmapByDegree(bmpDepth, 90); //旋转操作

                File imgFileDepth = new File(folderPath, profile + "grayFrame-" + currentDateAndTime + ".png");
                try {
                    FileOutputStream out4 = new FileOutputStream(imgFileDepth);
                    newbmpDepth.compress(Bitmap.CompressFormat.PNG, 100, out4);
                    out4.flush();
                    out4.close();
                    } catch (Exception e) {
                    e.printStackTrace();
                    }

                //CSV-getData()
                short[][] depthArray = depth16ToDepthRangeArray(return_buff_Depth, mDepth.cols(), mDepth.rows());
                File fDepth = new File(folderPath, profile + "getData-" + currentDateAndTime + ".csv");
                Path path = Paths.get(fDepth.toURI());
                try {
                    Files.write(path, toDepthRows(depthArray, depthFrame.getHeight(), depthFrame.getWidth()));
                } catch (IOException e) {
                    e.printStackTrace();
                }

                //CSV-getDistance()
//                float[][] deptValue = new float[depthFrame.getWidth()][depthFrame.getHeight()];
//                for (int y = 0; y < depthFrame.getHeight(); y++) {
//                    for (int x = 0; x < depthFrame.getWidth(); x++) {
//                        deptValue[x][y] = depthFrame.getDistance(x, y);
//                    }
//                }

                /**
                 *  矩阵转置
                 *  x, y遍历未转置前，xx,yy遍历转置后, 原点在左上角
                 */
                float[][] deptValue = new float[depthFrame.getHeight()][depthFrame.getWidth()];
                for (int y = 0, yy = 0; y < depthFrame.getWidth(); y++, yy++) {
                    for (int x = depthFrame.getHeight()-1, xx = 0; x >= 0; x--, xx++){
                        deptValue[xx][yy] = depthFrame.getDistance(y, x);
                    }
                }

                File fDepth2 = new File(folderPath, profile + "getDistance-" + currentDateAndTime + ".csv");
                Path path2 = Paths.get(fDepth2.toURI());
                try {
                    Files.write(path2, toDepthRows(deptValue, depthFrame.getHeight(), depthFrame.getWidth()));
                } catch (IOException e) {
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

    public void printdata(byte[] data) {
        File file = new File("/storage/emulated/0/graduateDesign/test.txt");
        try {
            FileWriter fw = new FileWriter(file);
            for (byte i : data) {
                fw.write(String.valueOf(i) + " ");
            }
            fw.flush();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }

    //CSV-getData()
    public short[][] depth16ToDepthRangeArray(byte[] return_buff_Depth, int w, int h) {
        ByteBuffer byteBuffer = ByteBuffer.wrap(return_buff_Depth);
        byteBuffer = byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
        ShortBuffer sBuffer = byteBuffer.asShortBuffer();
        short[][] depth = new short[w][h];
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                short depthSample = sBuffer.get(); //depth16[y*width + x];
                short depthRange = (short) (depthSample & 0x1FFF);
//                short depthConfidence = (short) ((depthSample >> 13) & 0x7);
                depth[x][y] = depthRange;
            }
        }
        //转置操作
        short[][] newDepth = new short[h][w];
        for (int y = 0, yy = 0; y < w; y++, yy++) {
            for (int x = h-1, xx = 0; x >= 0; x--, xx++){
                newDepth[xx][yy] = depth[y][x];
            }
        }
        return newDepth;
    }

    //CSV-getData()
    protected List<String> toDepthRows (short[][] depth, int w, int h) {
        List<String> mDepth = new ArrayList<>(w);
        for (int y = 0; y < h; y++) {
            StringBuffer sb = new StringBuffer();
            String depthString;
            for (int x = 0; x < w; x++) {
                double val = depth[x][y]*0.001;
                sb.append(val).append(",");
            }
            depthString = sb.toString();
            mDepth.add(depthString);
        }
        return mDepth;
    }

    //CSV-getDistance()
    protected List<String> toDepthRows (float[][] depth, int w, int h) {
        List<String> mDepth = new ArrayList<>(w);
        for (int y = 0; y < h; y++) {
            StringBuffer sb = new StringBuffer();
            String depthString;
            for (int x = 0; x < w; x++) {
                float val = depth[x][y];
                sb.append(val).append(",");
            }
            depthString = sb.toString();
            mDepth.add(depthString);
        }
        return mDepth;
    }

    public Bitmap rotateBitmapByDegree(Bitmap bm, int degree) {
        Bitmap returnBm = null;
        Matrix matrix = new Matrix();
        matrix.postRotate(degree);
        try {
            returnBm = Bitmap.createBitmap(bm, 0, 0, bm.getWidth(),
                    bm.getHeight(), matrix, true);
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        }
        if (returnBm == null) {
            returnBm = bm;
        }
        if (bm != returnBm) {
            bm.recycle();
        }
        return returnBm;
    }

}