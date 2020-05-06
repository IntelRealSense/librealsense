package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;

import java.util.HashMap;
import java.util.List;
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

    private boolean statsToggle = false;
    private boolean mShow3D = false;

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
                toggleStats();
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
    }

    @Override
    protected void onPause() {
        super.onPause();

        clearLables();
        if(mStreamer != null)
            mStreamer.stop();
        if(mGLSurfaceView != null)
            mGLSurfaceView.clear();
    }

    public void onRadioButtonClicked(View view) {
        // TODO - Ariel - Move touch logic into controls fragment. Only Sensor's logic should stay here.
        // TODO - Ariel - Add more controls
        boolean checked = ((RadioButton) view).isChecked();

        if(!checked)
            return;
        RsContext ctx = new RsContext();
        try(DeviceList devices = ctx.queryDevices()) {
            try(Device device = devices.createDevice(0)){
                if(device == null)
                    return;
                List<Sensor> sensors = device.querySensors();
                for(Sensor s : sensors){
                    if(s.supports(Option.EMITTER_ENABLED)) {
                        switch(view.getId()) {
                            case R.id.radio_no_projector:{
                                setOption(s, Option.EMITTER_ENABLED, 0);
                                break;
                            }
                            case R.id.radio_laser:{
                                setOption(s, Option.EMITTER_ENABLED, 1);
                                break;
                            }
                            case R.id.radio_laser_auto:{
                                setOption(s, Option.EMITTER_ENABLED, 2);
                                break;
                            }
                            case R.id.radio_led:{
                                setOption(s, Option.EMITTER_ENABLED, 3);
                                break;
                            }
                        }
                    }
                    if(s.supports(Option.HARDWARE_PRESET)) {
                        switch(view.getId()) {
                            case R.id.radio_custom:{
                                setOption(s, Option.HARDWARE_PRESET, 0);
                                break;
                            }
                            case R.id.radio_burst:{
                                setOption(s, Option.HARDWARE_PRESET, 2);
                                break;
                            }
                        }
                    }

                }
            } catch(Exception e){
                Log.e(TAG, "Failed to set controls: " + e.getMessage());
                Toast.makeText(this, "Failed to set controls", Toast.LENGTH_LONG).show();
            }
        }
    }

    private void setOption(Sensor s, Option option, int val) {
        if(s.supports(option))
            s.setValue(option, val);
        else
            Toast.makeText(this, "This control is not supported by this device", Toast.LENGTH_LONG).show();
    }
}
