package com.example.realsense_app;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.ToggleButton;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
        }

        this.mStreamDepth = this.findViewById(R.id.streamDepth);
        this.mStreamColor = this.findViewById(R.id.streamColor);
        this.mStreamIR = this.findViewById(R.id.streamIR);

        MainActivity.this.mStreamDepth.setEnabledForUI(false);
        MainActivity.this.mStreamColor.setEnabledForUI(false);
        MainActivity.this.mStreamIR.setEnabledForUI(false);

        this.mLogView = this.findViewById(R.id.logView);

        this.mSwitchButtonPower = this.findViewById(R.id.toggleButtonPower);
        this.mSwitchButtonPower.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    MainActivity.this.mStreamDepth.setEnabledForUI(true);
                    MainActivity.this.mStreamColor.setEnabledForUI(true);
                    MainActivity.this.mStreamIR.setEnabledForUI(true);

                    init();

                    if (MainActivity.this.mStreamDepth.isStreamEnabledByDefault()) {
                        MainActivity.this.mStreamDepth.setStreamEnabled(true);
                    }
                    if (MainActivity.this.mStreamColor.isStreamEnabledByDefault()) {
                        MainActivity.this.mStreamColor.setStreamEnabled(true);
                    }
                    if (MainActivity.this.mStreamIR.isStreamEnabledByDefault()) {
                        MainActivity.this.mStreamIR.setStreamEnabled(true);
                    }

                    MainActivity.this.mLogView.setText(logMessage());

                    MainActivity.this.mSwitchButtonPlay.setEnabled(true);
                } else {
                    MainActivity.this.mSwitchButtonPlay.setEnabled(false);

                    cleanup();

                    MainActivity.this.mStreamDepth.setStreamEnabled(false);
                    MainActivity.this.mStreamColor.setStreamEnabled(false);
                    MainActivity.this.mStreamIR.setStreamEnabled(false);

                    MainActivity.this.mStreamDepth.setEnabledForUI(false);
                    MainActivity.this.mStreamColor.setEnabledForUI(false);
                    MainActivity.this.mStreamIR.setEnabledForUI(false);
                }
            }
        });

        this.mSwitchButtonPlay = this.findViewById(R.id.toggleButtonPlay);
        this.mSwitchButtonPlay.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    MainActivity.this.mSwitchButtonPower.setEnabled(false);

                    Play();
                } else {
                    Stop();

                    MainActivity.this.mSwitchButtonPower.setEnabled(true);
                }
            }
        });

        mOnUpdateStreamProfileListListener = new StreamSurface.OnUpdateStreamProfileListListener() {

            @Override
            public String onUpdateStreamProfileList(StreamSurface v, String streamName, String resolution, String sfps) {
                Log.d("RS", "onUpdateStreamProfileList RES:" + resolution + " FPS:" + sfps);
                int width = -1; int height = -1; int fps = -1;
                if (resolution != null) {
                    String[] res = resolution.split("x");
                    if (res.length == 2) {
                        width = Integer.parseInt(res[0]);
                        height = Integer.parseInt(res[1]);
                    }
                }

                if (sfps!= null && !sfps.isEmpty()) {
                    fps = Integer.parseInt(sfps);
                }
                return MainActivity.getProfileList(streamName, width, height, fps);
            }
        };

        this.mStreamDepth.setOnUpdateStreamProfileListListener(mOnUpdateStreamProfileListListener);
        this.mStreamColor.setOnUpdateStreamProfileListListener(mOnUpdateStreamProfileListListener);
        this.mStreamIR.setOnUpdateStreamProfileListListener(mOnUpdateStreamProfileListListener);
    }

    private void Play() {
        this.EnableStream(this.mStreamDepth);
        this.EnableStream(this.mStreamColor);
        this.EnableStream(this.mStreamIR);

        play();
    }
    private void Stop() {
        stop();
    }

    private void EnableStream(StreamSurface surface) {
        if (!surface.isStreamEnable()) return;

        StreamSurface.StreamConfig config = surface.getCurrentConfig();
        int type =  rs2.StreamFromString(config.mType);
        //surface.getStreamView().getHolder().getSurface();
        Log.d("MAINACTIVITY", String.format("Stream: %s w %d h %d fps %d Format: %s",
                config.mType, config.mWidth, config.mHeight, config.mFPS, config.mFormat));
        enableStream(rs2.StreamFromString(config.mType),
                config.mWidth, config.mHeight, config.mFPS,
                rs2.FormatFromString(config.mFormat),
                surface.getStreamView().getHolder().getSurface()
                );
    }

    StreamSurface.OnUpdateStreamProfileListListener mOnUpdateStreamProfileListListener;

    StreamSurface mStreamDepth;
    StreamSurface mStreamColor;
    StreamSurface mStreamIR;

    TextView mLogView;

    ToggleButton mSwitchButtonPower;
    ToggleButton mSwitchButtonPlay;

    public native static void init();
    public native static void cleanup();
    public native static void enableStream(int stream, int width, int height, int fps, int format, Surface surface);
    public native static void play();
    public native static void stop();

    public native static String getProfileList(String type, int width, int height, int fps);

    public native static String logMessage();
}
