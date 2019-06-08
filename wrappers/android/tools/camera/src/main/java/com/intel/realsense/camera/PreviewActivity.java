package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;

public class PreviewActivity extends AppCompatActivity {
    private static final String TAG = "librs camera pr";

    private static final int PLAYBACK_REQUEST_CODE = 0;
    private static final int SETTINGS_REQUEST_CODE = 1;

    private GLRsSurfaceView mGLSurfaceView;
    private FloatingActionButton mStartRecordFab;
    private TextView mPlaybackButton;
    private TextView mSettingsButton;
    private TextView mStatisticsButton;

    private TextView mStatsView;

    private Streamer mStreamer;
    private Colorizer mColorizer = new Colorizer();

    private StreamingStats mStreamingStats  = new StreamingStats();

    private boolean statsToggle = false;

    public synchronized void toggleStats(){
        statsToggle = !statsToggle;
        if(statsToggle){
            mGLSurfaceView.setVisibility(View.GONE);
            mStatsView.setVisibility(View.VISIBLE);
            mStatisticsButton.setText("Preview");
        }
        else {
            mGLSurfaceView.setVisibility(View.VISIBLE);
            mStatsView.setVisibility(View.GONE);
            mStatisticsButton.setText("Statistics");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_preview);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mGLSurfaceView = findViewById(R.id.glSurfaceView);
        mStatsView = findViewById(R.id.streaming_stats_text);
        mStartRecordFab = findViewById(R.id.start_record_fab);
        mPlaybackButton = findViewById(R.id.preview_playback_button);
        mSettingsButton = findViewById(R.id.preview_settings_button);
        mStatisticsButton = findViewById(R.id.preview_stats_button);

        mStartRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, RecordingActivity.class);
                startActivity(intent);
                finish();
            }
        });
        mPlaybackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, PlaybackActivity.class);
                startActivityForResult(intent, PLAYBACK_REQUEST_CODE);
            }
        });
        mSettingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
                startActivityForResult(intent, SETTINGS_REQUEST_CODE);
            }
        });
        mStatisticsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleStats();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        mStreamer = new Streamer(this, true, new Streamer.Listener() {
            @Override
            public void config(Config config) {

            }

            @Override
            public void onFrameset(FrameSet frameSet) {
                mStreamingStats.onFrameset(frameSet);
                if(statsToggle){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mStatsView.setText(mStreamingStats.prettyPrint());
                        }
                    });
                }
                else{
                    try (FrameSet processed = frameSet.applyFilter(mColorizer)) {
                        mGLSurfaceView.upload(processed);
                    }
                }
            }
        });

        try {
            mGLSurfaceView.clear();
            mStreamer.start();
        }
        catch (Exception e){
            if(mStreamer != null)
                mStreamer.stop();
            Log.e(TAG, e.getMessage());
            Toast.makeText(this, "Invalid configuration selected", Toast.LENGTH_LONG).show();
            Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
            startActivityForResult(intent, SETTINGS_REQUEST_CODE);
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
        Intent intent = new Intent(this, DetachedActivity.class);
        startActivity(intent);
        finish();
    }
}
