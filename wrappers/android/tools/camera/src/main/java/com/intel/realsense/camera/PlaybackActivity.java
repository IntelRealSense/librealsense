package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.WindowManager;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;

import java.io.File;

public class PlaybackActivity extends AppCompatActivity {
    private static final String TAG = "librs camera pb";

    private static final int OPEN_FILE_REQUEST_CODE = 0;
    private static final String FILE_PATH_KEY = "FILE_PATH_KEY";

    private String mFilePath;
    private Streamer mStreamer;
    private GLRsSurfaceView mGLSurfaceView;
    private Colorizer mColorizer = new Colorizer();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_playback);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mGLSurfaceView = findViewById(R.id.playbackGlSurfaceView);
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mFilePath == null){
            Intent intent = new Intent(this, FileBrowserActivity.class);
            intent.putExtra(getString(R.string.browse_folder), getString(R.string.realsense_folder) + File.separator + "video");
            startActivityForResult(intent, OPEN_FILE_REQUEST_CODE);
        }
        else{
            mStreamer = new Streamer(this,false, new Streamer.Listener() {
                @Override
                public void config(Config config) {
                    config.enableAllStreams();
                    config.enableDeviceFromFile(mFilePath);
                }

                @Override
                public void onFrameset(FrameSet frameSet) {
                    try (FrameSet processed = frameSet.applyFilter(mColorizer)) {
                        mGLSurfaceView.upload(processed);
                    }
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
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OPEN_FILE_REQUEST_CODE && resultCode == RESULT_OK) {
            if (data != null) {
                mFilePath = data.getStringExtra(getString(R.string.intent_extra_file_path));
            }
        }
        else{
            Intent intent = new Intent();
            setResult(RESULT_OK, intent);
            finish();
        }
    }

    @Override
    protected void onSaveInstanceState(final Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putString(FILE_PATH_KEY, mFilePath);
    }

    @Override
    protected void onRestoreInstanceState(final Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mFilePath = savedInstanceState.getString(FILE_PATH_KEY);
    }
}
