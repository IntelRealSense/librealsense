package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

public class PlaybackActivity extends AppCompatActivity {
    private static final String TAG = "librs camera pb";

    private static final int OPEN_FILE_REQUEST_CODE = 0;
    private static final String FILE_PATH_KEY = "FILE_PATH_KEY";

    private String mFilePath;
    private Streamer mStreamer;
    private GLRsSurfaceView mGLSurfaceView;

    private Map<Integer, TextView> mLabels;

    private boolean mShow3D = false;
    private TextView m3dButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_playback);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mGLSurfaceView = findViewById(R.id.playbackGlSurfaceView);
        m3dButton = findViewById(R.id.playback_3d_button);
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
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        mShow3D = sharedPref.getBoolean(getString(R.string.show_3d), false);
        m3dButton.setTextColor(mShow3D ? Color.YELLOW : Color.WHITE);
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
                    mGLSurfaceView.showPointcloud(mShow3D);
                    mGLSurfaceView.upload(frameSet);
                    Map<Integer, Pair<String, Rect>> rects = mGLSurfaceView.getRectangles();
                    printLables(rects);
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

        clearLables();

        if(mStreamer != null)
            mStreamer.stop();
        if(mGLSurfaceView != null)
            mGLSurfaceView.clear();
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
}
