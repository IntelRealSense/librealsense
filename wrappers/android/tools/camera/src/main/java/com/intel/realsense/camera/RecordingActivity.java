package com.intel.realsense.camera;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Bundle;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.view.View;
import android.view.WindowManager;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

public class RecordingActivity extends AppCompatActivity {
    private static final String TAG = "librs camera rec";

    private Streamer mStreamer;
    private GLRsSurfaceView mGLSurfaceView;

    private boolean mPermissionsGranted = false;

    private FloatingActionButton mStopRecordFab;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recording);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setupControls();

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PermissionsUtils.PERMISSIONS_REQUEST_WRITE);
            return;
        }

        mPermissionsGranted = true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PermissionsUtils.PERMISSIONS_REQUEST_WRITE);
            return;
        }

        mPermissionsGranted = true;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // handling device orientation changes to avoid interruption during recording

        // cleanup previous surface
        if(mGLSurfaceView != null) {
            mGLSurfaceView.clear();
            mGLSurfaceView.close();
        }

        // setup recording layout landscape or portrait automatically depends on orientation
        setContentView(R.layout.activity_recording);

        // setup layout controls
        setupControls();
    }

    private void setupControls() {
        mGLSurfaceView = findViewById(R.id.recordingGlSurfaceView);

        mStopRecordFab = findViewById(R.id.stopRecordFab);
        mStopRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(RecordingActivity.this, PreviewActivity.class);
                startActivity(intent);
                finish();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mPermissionsGranted){
            mStreamer = new Streamer(this,true, new Streamer.Listener() {
                @Override
                public void config(Config config) {
                    config.enableRecordToFile(getFilePath());
                }

                @Override
                public void onFrameset(FrameSet frameSet) {
                    mGLSurfaceView.upload(frameSet);
                }
            });
            try {
                mGLSurfaceView.clear();
                mStreamer.start();
            } catch (Exception e) {
                finish();
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if(mStreamer != null)
            mStreamer.stop();
        if(mGLSurfaceView != null)
            mGLSurfaceView.clear();
    }

    private String getFilePath(){
        File rsFolder = new File(getExternalFilesDir(null).getAbsolutePath() +
                File.separator + getString(R.string.realsense_folder));
        rsFolder.mkdir();
        File folder = new File(getExternalFilesDir(null).getAbsolutePath() +
                File.separator + getString(R.string.realsense_folder) + File.separator + "video");
        folder.mkdir();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss");
        String currentDateAndTime = sdf.format(new Date());
        File file = new File(folder, currentDateAndTime + ".bag");
        return file.getAbsolutePath();
    }
}
