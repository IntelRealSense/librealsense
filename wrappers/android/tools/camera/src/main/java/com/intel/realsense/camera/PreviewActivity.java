package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.WindowManager;

import com.intel.realsense.librealsense.GLRsSurfaceView;

public class PreviewActivity extends AppCompatActivity {
    private static final String TAG = "librs camera pr";

    private static final int PLAYBACK_REQUEST_CODE = 0;
    private static final int SETTINGS_REQUEST_CODE = 1;

    private FloatingActionButton mStartRecordFab;
    private FloatingActionButton mPlaybackFab;
    private FloatingActionButton mSettingsFab;

    private Streamer mStreamer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_preview);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mStartRecordFab = findViewById(R.id.startRecordFab);
        mPlaybackFab = findViewById(R.id.playbackFab);
        mSettingsFab = findViewById(R.id.settingsFab);

        mStartRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, RecordingActivity.class);
                startActivity(intent);
                finish();
            }
        });
        mPlaybackFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, PlaybackActivity.class);
                startActivityForResult(intent, PLAYBACK_REQUEST_CODE);
            }
        });
        mSettingsFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
                startActivityForResult(intent, SETTINGS_REQUEST_CODE);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        mStreamer = new Streamer(this, (GLRsSurfaceView) findViewById(R.id.glSurfaceView), null);
        mStreamer.start();
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
        Intent intent = new Intent(this, DetachedActivity.class);
        startActivity(intent);
        finish();
    }
}
