package com.intel.realsense.playback;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.PipelineProfile;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "librs playback example";
    private static final int READ_REQUEST_CODE = 0;
    private Uri mUri;
    private GLRsSurfaceView mGLSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mGLSurfaceView = findViewById(R.id.glSurfaceView);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 0);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLSurfaceView.close();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 0);
            return;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mUri == null){
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, READ_REQUEST_CODE);
        } else{
            init();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        mStreaming.interrupt();
        if(mStreaming.isAlive()) {
            try {
                mStreaming.join(1000);
                mGLSurfaceView.clear();
            } catch (InterruptedException e) {
                Log.e(TAG, e.getMessage());
            }
        }
    }

    private void init(){
        mStreaming.start();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == READ_REQUEST_CODE && resultCode == RESULT_OK) {
            if (data != null) {
                mUri = data.getData();
            }
        }
    }

    Thread mStreaming = new Thread() {
        @Override
        public void run() {
            String filePath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + mUri.getPath().split(":")[1];
            try(Colorizer colorizer = new Colorizer()) {
                try (Config config = new Config()) {
                    config.enableDeviceFromFile(filePath);
                    try (Pipeline pipeline = new Pipeline()) {
                        try {
                            // try statement needed here to release resources allocated by the Pipeline:start() method
                            try (PipelineProfile pp = pipeline.start(config)) {}
                            while (!mStreaming.isInterrupted()) {
                                try (FrameSet frames = pipeline.waitForFrames(1000)) {
                                    try (FrameSet processed = frames.applyFilter(colorizer)) {
                                        mGLSurfaceView.upload(processed);
                                    }
                                }
                            }
                            pipeline.stop();
                        } catch (Exception e) {
                            Log.e(TAG, "streaming, error: " + e.getMessage());
                        }
                    }
                }
            }
        }
    };
}
