package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;

import java.util.HashMap;
import java.util.Map;

public class PreviewActivity extends AppCompatActivity {
    private static final String TAG = "librs camera preview";

    private GLRsSurfaceView mGLSurfaceView;
    private FloatingActionButton mStartRecordFab;
    private TextView mPlaybackButton;
    private TextView mSettingsButton;
    private TextView mStatisticsButton;
    private TextView m3dButton;
    private TextView mControlsButton;

    private TextView mStatsView;
    private Map<Integer, TextView> mLabels;

    private Streamer mStreamer;
    private StreamingStats mStreamingStats;

    private FwLogsThread mFwLogsThread;
    private boolean mFwLogsRunning = false;

    private boolean statsToggle = false;
    private boolean mShow3D = false;

    boolean keepalive = true;

    public synchronized void updateStats(){
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

        keepalive = true;

        Intent intent = this.getIntent();

        if (intent != null && intent.getExtras() != null ) {
            keepalive = intent.getExtras().getBoolean("keepalive");
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setupControls();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // handling device orientation changes
        // upon device orientation change, actitity will not go through default route
        // to destroy and recreate the preview activity
        // instead to refresh the appropriate layout and control here
        // so that device streaming is not interrupted

        // cleanup previous surface
        if(mGLSurfaceView != null) {
            mGLSurfaceView.clear();
            mGLSurfaceView.close();
        }

        // setup preview layout landscape or portrait automatically depends on orientation
        setContentView(R.layout.activity_preview);

        // update layout controls
        setupControls();
        updateStats();
    }

    private void setupControls()
    {
        mGLSurfaceView = findViewById(R.id.glSurfaceView);

        mStatsView = findViewById(R.id.streaming_stats_text);
        mStartRecordFab = findViewById(R.id.start_record_fab);
        mPlaybackButton = findViewById(R.id.preview_playback_button);
        mSettingsButton = findViewById(R.id.preview_settings_button);
        mStatisticsButton = findViewById(R.id.preview_stats_button);
        m3dButton = findViewById(R.id.preview_3d_button);
        mControlsButton = findViewById(R.id.controls_button);

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
        m3dButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mGLSurfaceView.setVisibility(View.GONE);
                mGLSurfaceView.clear();
                clearLables();
                mShow3D = !mShow3D;
                mGLSurfaceView.showPointcloud(mShow3D);

                m3dButton.setTextColor(mShow3D ? Color.YELLOW : Color.WHITE);
                mGLSurfaceView.setVisibility(View.VISIBLE);
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean(getString(R.string.show_3d), mShow3D);
                editor.commit();
            }
        });
        mControlsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ControlsDialog cd = new ControlsDialog();
                cd.setCancelable(true);
                cd.show(getFragmentManager(), null);
            }
        });
        mStatisticsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                statsToggle = !statsToggle;
                updateStats();
            }
        });
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        mShow3D = sharedPref.getBoolean(getString(R.string.show_3d), false);
        m3dButton.setTextColor(mShow3D ? Color.YELLOW : Color.WHITE);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLSurfaceView.close();
    }

    private synchronized Map<Integer, TextView> createLabels(Map<Integer, Pair<String, Rect>> rects){
        if(rects == null)
            return null;
        mLabels = new HashMap<>();

        final RelativeLayout rl = findViewById(R.id.labels_layout);
        for(Map.Entry<Integer, Pair<String, Rect>> e : rects.entrySet()){
            TextView tv = new TextView(getApplicationContext());
            ViewGroup.LayoutParams lp = new RelativeLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);
            tv.setLayoutParams(lp);
            tv.setTextColor(Color.parseColor("#ffffff"));
            tv.setTextSize(14);
            rl.addView(tv);
            mLabels.put(e.getKey(), tv);
        }
        return mLabels;
    }

    private void printLables(final Map<Integer, Pair<String, Rect>> rects){
        if(rects == null)
            return;
        final Map<Integer, String> lables = new HashMap<>();
        if(mLabels == null)
            mLabels = createLabels(rects);
        for(Map.Entry<Integer, Pair<String, Rect>> e : rects.entrySet()){
            lables.put(e.getKey(), e.getValue().first);
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for(Map.Entry<Integer,TextView> e : mLabels.entrySet()){
                    Integer uid = e.getKey();
                    if(rects.get(uid) == null)
                        continue;
                    Rect r = rects.get(uid).second;
                    TextView tv = e.getValue();
                    tv.setX(r.left);
                    tv.setY(r.top);
                    tv.setText(lables.get(uid));
                }
            }
        });
    }

    private void clearLables(){
        if(mLabels != null){
            for(Map.Entry<Integer, TextView> label : mLabels.entrySet())
                label.getValue().setVisibility(View.GONE);
            mLabels = null;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(keepalive == false)
        {
            return;
        }

        mStreamingStats  = new StreamingStats();
        mStreamer = new Streamer(this, true, new Streamer.Listener() {
            @Override
            public void config(Config config) {

            }

            @Override
            public void onFrameset(final FrameSet frameSet) {
                mStreamingStats.onFrameset(frameSet);
                if(statsToggle){
                    clearLables();
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mStatsView.setText(mStreamingStats.prettyPrint());
                        }
                    });
                }
                else{
                    mGLSurfaceView.showPointcloud(mShow3D);
                    mGLSurfaceView.upload(frameSet);
                    Map<Integer, Pair<String, Rect>> rects = mGLSurfaceView.getRectangles();
                    printLables(rects);
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

        resumeBackgroundTasks();
    }

    @Override
    protected void onPause() {
        super.onPause();

        clearLables();
        if(mStreamer != null)
            mStreamer.stop();
        if(mGLSurfaceView != null)
            mGLSurfaceView.clear();

        pauseBackgroundTasks();
    }

    private synchronized void resumeBackgroundTasks() {
        resumeFwLogger();
    }

    private synchronized void pauseBackgroundTasks() {
        pauseFwLogger();
    }

    private synchronized void resumeFwLogger() {
        if (!mFwLogsRunning)
        {
            SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
            boolean fw_logging_enabled = sharedPref.getBoolean(getString(R.string.fw_logging), false);
            String fw_logging_file_path = sharedPref.getString(getString(R.string.fw_logging_file_path), "");

            if (fw_logging_enabled) {
                mFwLogsThread = new FwLogsThread();
                if(!fw_logging_file_path.equals("")){
                    mFwLogsThread.init(fw_logging_file_path);
                }
                mFwLogsThread.start();
                mFwLogsRunning = true;
            }
        }
    }

    private synchronized void pauseFwLogger(){
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        boolean fw_logging_enabled = sharedPref.getBoolean(getString(R.string.fw_logging), false);
        if (fw_logging_enabled) {
            mFwLogsThread.stopLogging();
        }
        if(mFwLogsThread != null && mFwLogsThread.isAlive()) {
            mFwLogsThread.interrupt();
        }
        mFwLogsRunning = false;
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);

        if (intent != null && intent.getExtras() != null ) {
            keepalive = intent.getExtras().getBoolean("keepalive");
        }

        // destroy activity if requested
        if(keepalive == false)
        {
            PreviewActivity.this.finish();
        }
    }

}
