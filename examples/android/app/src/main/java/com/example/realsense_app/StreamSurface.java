package com.example.realsense_app;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.Nullable;
import android.support.constraint.ConstraintLayout;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;

import java.util.Locale;

public class StreamSurface extends FrameLayout {

    public interface OnUpdateStreamProfileListListener {
        String onUpdateStreamProfileList(StreamSurface v, String streamName, String resolution, String fps);
    }
    OnUpdateStreamProfileListListener mOnUpdateStreamProfileListListener;
    public void setOnUpdateStreamProfileListListener(OnUpdateStreamProfileListListener listener) {
        if (this.mOnUpdateStreamProfileListListener == null) {
            if (this.mStreamEnabled.isChecked()) {
                String list = listener.onUpdateStreamProfileList(
                        StreamSurface.this,
                        StreamSurface.this.mStreamTypeList.getSelectedItem().toString(),
                        null,
                        null);
                if (!list.isEmpty())
                    setStreamResolutionList(list);
            }
        }
        this.mOnUpdateStreamProfileListListener = listener;
    }

    public StreamSurface(Context context) {
        this(context, null);
    }

    public StreamSurface(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public StreamSurface(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        this.mCurrentConfig = new StreamConfig();

        init(context, attrs, defStyleAttr);
    }

    private void init(Context context, AttributeSet attrs, int defStyle) {

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.stream_surface, this);

        this.mStreamEnabled = this.findViewById(R.id.checkBoxEnabled);
        this.mStreamTypeList = this.findViewById(R.id.spinnerStream);
        this.mStreamResolution = this.findViewById(R.id.spinnerResolution);
        this.mStreamFPS = this.findViewById(R.id.spinnerFPS);
        this.mStreamFormat = this.findViewById(R.id.spinnerFormat);

        this.mStreamView = this.findViewById(R.id.surfaceView);

        this.mDefaultConfig = new StreamConfig();

        // Load attributes
        final TypedArray a = getContext().obtainStyledAttributes(
                attrs, R.styleable.StreamSurface, defStyle, 0);

        this.mDefaultConfig.mType = a.getString(R.styleable.StreamSurface_DefaultStream);
        this.mDefaultConfig.mFormat = a.getString(R.styleable.StreamSurface_DefaultFormat);
        this.mDefaultConfig.mWidth = a.getInt(R.styleable.StreamSurface_DefaultWidth, 640);
        this.mDefaultConfig.mHeight = a.getInt(R.styleable.StreamSurface_DefaultHeight, 480);
        this.mDefaultConfig.mFPS = a.getInt(R.styleable.StreamSurface_DefaultFPS, 30);

        this.mDefaultEnabled = a.getBoolean(R.styleable.StreamSurface_DefaultEnable, false);
        //this.mStreamEnabled.setChecked(a.getBoolean(R.styleable.StreamSurface_DefaultEnable, false));

        this.initSpinner(context, this.mStreamTypeList, this.mDefaultConfig.mType, this.mDefaultConfig.mType);
        this.initSpinner(context, this.mStreamFormat, this.mDefaultConfig.mFormat, this.mDefaultConfig.mFormat);
        this.initSpinner(context, this.mStreamResolution, String.format(Locale.getDefault(), "%dx%d", this.mDefaultConfig.mWidth, this.mDefaultConfig.mHeight), String.format(Locale.getDefault(), "%dx%d", this.mDefaultConfig.mWidth, this.mDefaultConfig.mHeight));
        this.initSpinner(context, this.mStreamFPS, String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mFPS), String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mFPS));

        ViewGroup.LayoutParams params = this.mStreamView.getLayoutParams();
        params.width = this.mDefaultConfig.mWidth;
        params.height = this.mDefaultConfig.mHeight;
        this.mStreamView.setLayoutParams(params);

        a.recycle();

        this.mStreamEnabled.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    if (StreamSurface.this.mOnUpdateStreamProfileListListener != null) {
                        String list = StreamSurface.this.mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                                StreamSurface.this,
                                StreamSurface.this.mStreamTypeList.getSelectedItem().toString(),
                                null,
                                null);
                        Log.d("RS", "mStreamEnabled onCheckedChanged " + StreamSurface.this.mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                        setStreamResolutionList(list);
                    }
                }
            }
        });

        this.mStreamResolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (StreamSurface.this.mOnUpdateStreamProfileListListener != null) {
                    String list = StreamSurface.this.mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            StreamSurface.this.mStreamTypeList.getSelectedItem().toString(),
                            StreamSurface.this.mStreamResolution.getSelectedItem().toString(),
                            null);
                    Log.d("RS", "mStreamResolution onItemSelected " + StreamSurface.this.mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamFPSList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                Log.d("RS", "mStreamResolution onNothingSelected");
            }
        });

        this.mStreamFPS.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (StreamSurface.this.mOnUpdateStreamProfileListListener != null) {
                    String list = StreamSurface.this.mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            StreamSurface.this.mStreamTypeList.getSelectedItem().toString(),
                            StreamSurface.this.mStreamResolution.getSelectedItem().toString(),
                            StreamSurface.this.mStreamFPS.getSelectedItem().toString());
                    Log.d("RS", "mStreamFPS onItemSelected " + StreamSurface.this.mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamFormatList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                Log.d("RS", "mStreamFPS onNothingSelected");
            }
        });

    }

    private void initSpinner(Context context, Spinner spinner, @Nullable String list, String defValue) {
        if (list == null) return;

        String[] _list = list.split(",");
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item, _list);
        spinner.setAdapter(adapter);

        int defPosition = adapter.getPosition(defValue);
        if (defPosition >= 0) {
            spinner.setSelection(defPosition);
        }
    }

    private void getWidthAndHeightFromUI() {
        String resolution = this.mStreamResolution.getSelectedItem().toString();
        String[] _list = resolution.split("x");
        if (_list.length == 2) {
            this.mCurrentConfig.mWidth = Integer.parseInt(_list[0]);
            this.mCurrentConfig.mHeight = Integer.parseInt(_list[1]);
        } else {
            this.mCurrentConfig.mWidth = this.mDefaultConfig.mWidth;
            this.mCurrentConfig.mHeight = this.mDefaultConfig.mHeight;
        }
    }
    private void getFPSFromUI() {
        String fps = this.mStreamFPS.getSelectedItem().toString();
        if (fps.isEmpty()) {
            this.mCurrentConfig.mFPS = this.mDefaultConfig.mFPS;
        } else {
            this.mCurrentConfig.mFPS = Integer.parseInt(fps);
        }
    }

    public String getCurrentResolution() {
        return this.mStreamResolution.getSelectedItem().toString();
    }
    public String getCurrentFPS() {
        return this.mStreamFPS.getSelectedItem().toString();
    }
    public void setStreamResolutionList(String list) {
        this.initSpinner(this.mStreamResolution.getContext(), this.mStreamResolution, list, String.format(Locale.getDefault(), "%dx%d", this.mDefaultConfig.mWidth, this.mDefaultConfig.mHeight));
    }
    public void setStreamFPSList(String list) {
        this.initSpinner(this.mStreamFPS.getContext(), this.mStreamFPS, list, String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mFPS));
    }
    public void setStreamFormatList(String list) {
        this.initSpinner(this.mStreamFormat.getContext(), this.mStreamFormat, list, this.mDefaultConfig.mFormat);
    }

    public boolean isStreamEnable() {
        return this.mStreamEnabled.isChecked();
    }

    public boolean isStreamEnabledByDefault() {
        return this.mDefaultEnabled;
    }

    public void setStreamEnabled(boolean flag) {
        this.mStreamEnabled.setChecked(flag);
    }

    public void setEnabledForUI(boolean flag) {
        this.mStreamEnabled.setEnabled(flag);
    }

    public StreamConfig getCurrentConfig() {
        this.mCurrentConfig.mType = this.mStreamTypeList.getSelectedItem().toString();
        this.mCurrentConfig.mFormat = this.mStreamFormat.getSelectedItem().toString();
        getWidthAndHeightFromUI();
        getFPSFromUI();

        ViewGroup.LayoutParams params = this.mStreamView.getLayoutParams();
        params.width = this.mCurrentConfig.mWidth;
        params.height = this.mCurrentConfig.mHeight;
        this.mStreamView.setLayoutParams(params);

        return this.mCurrentConfig;
    }

    public SurfaceView getStreamView() {
        return this.mStreamView;
    }

    boolean mDefaultEnabled;
    StreamConfig mDefaultConfig;
    StreamConfig mCurrentConfig;

    CheckBox mStreamEnabled;
    Spinner mStreamTypeList;
    Spinner mStreamResolution;
    Spinner mStreamFPS;
    Spinner mStreamFormat;

    SurfaceView mStreamView;

    public class StreamConfig {
        public String mType;
        public String mFormat;
        public int mWidth;
        public int mHeight;
        public int mFPS;
    }
}
