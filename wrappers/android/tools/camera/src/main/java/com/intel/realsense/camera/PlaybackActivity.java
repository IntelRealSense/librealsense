package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.WindowManager;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.GLRsSurfaceView;

public class PlaybackActivity extends AppCompatActivity {
    private static final String TAG = "librs camera pb";

    private static final int OPEN_FILE_REQUEST_CODE = 0;
    private static final String FILE_PATH_KEY = "FILE_PATH_KEY";

    private String mFilePath;
    private Streamer mStreamer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_playback);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mFilePath == null){
            Intent intent = new Intent(this, FileBrowserActivity.class);
            startActivityForResult(intent, OPEN_FILE_REQUEST_CODE);
        }
        else{
            mStreamer = new Streamer(this, (GLRsSurfaceView) findViewById(R.id.playbackGlSurfaceView), new Streamer.Listener() {
                @Override
                public void config(Config config) {
                    config.enableAllStreams();
                    config.enableDeviceFromFile(mFilePath);
                }
            });
            mStreamer.start();
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
