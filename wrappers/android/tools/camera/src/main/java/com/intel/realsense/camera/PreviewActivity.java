package com.intel.realsense.camera;

import android.content.Intent;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.renderscript.Float3;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.MotionFrame;
import com.intel.realsense.librealsense.StreamProfile;

import java.util.HashMap;
import java.util.Map;

public class PreviewActivity extends AppCompatActivity {
    private static final String TAG = "librs camera preview";

    private GLRsSurfaceView mGLSurfaceView;
    private FloatingActionButton mStartRecordFab;
    private TextView mPlaybackButton;
    private TextView mSettingsButton;
    private TextView mStatisticsButton;

    private TextView mStatsView;
    private Map<Integer, TextView> mLabels;

    private Streamer mStreamer;
    private Colorizer mColorizer = new Colorizer();

    private StreamingStats mStreamingStats;

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
                startActivity(intent);
            }
        });
        mSettingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
                startActivity(intent);
            }
        });
        mStatisticsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleStats();
            }
        });
    }

    private synchronized Map<Integer, TextView> createLabels(FrameSet frameSet){
        mLabels = new HashMap<>();

        final RelativeLayout rl = findViewById(R.id.labels_layout);

        frameSet.foreach(new FrameCallback() {
            @Override
            public void onFrame(Frame f) {
                TextView tv = new TextView(getApplicationContext());
                ViewGroup.LayoutParams lp = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT);
                tv.setLayoutParams(lp);
                tv.setTextColor(Color.parseColor("#ffffff"));
                tv.setTextSize(14);
                rl.addView(tv);
                StreamProfile p = f.getProfile();
                mLabels.put(p.getUniqueId(), tv);
            }
        });
        return mLabels;
    }

    private void printLables(FrameSet frames, final Map<Integer, Rect> rects){
        final Map<Integer, String> lables = new HashMap<>();
        if(mLabels == null)
            mLabels = createLabels(frames);
        frames.foreach(new FrameCallback() {
            @Override
            public void onFrame(Frame f) {
                StreamProfile p = f.getProfile();
                if(f.is(Extension.MOTION_FRAME)) {
                    MotionFrame mf = f.as(Extension.MOTION_FRAME);
                    Float3 d = mf.getMotionData();
                    lables.put(p.getUniqueId(),p.getType().name() +
                            " [ X: " + String.format("%+.2f", d.x) +
                            ", Y: " + String.format("%+.2f", d.y) +
                            ", Z: " + String.format("%+.2f", d.z) + " ]");
                }
                else{
                    lables.put(p.getUniqueId(), p.getType().name());
                }
            }
        });
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for(Map.Entry<Integer,TextView> e : mLabels.entrySet()){
                    Integer uid = e.getKey();
                    Rect r = rects.get(uid);
                    if(r == null)
                        continue;
                    TextView tv = e.getValue();
                    tv.setX(r.left);
                    tv.setY(r.top);
                    tv.setText(lables.get(uid));
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        mStreamingStats  = new StreamingStats();
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
                        Map<Integer, Rect> rects = mGLSurfaceView.getRectangles();
                        printLables(processed, rects);
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
            Toast.makeText(this, "Failed to set streaming configuration ", Toast.LENGTH_LONG).show();
            Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
            startActivity(intent);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        for(Map.Entry<Integer,TextView> e : mLabels.entrySet()){
            TextView tv = e.getValue();
            tv.setVisibility(View.GONE);
        }
        mLabels = null;
        if(mStreamer != null)
            mStreamer.stop();
    }
}
