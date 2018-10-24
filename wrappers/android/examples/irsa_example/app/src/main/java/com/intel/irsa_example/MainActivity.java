// License: Apache 2.0. See LICENSE file in root directory.
//
// This is a "Hello-world" example to illustrate how to using android wrapper
// for development activities with librealsense for Android based device

package com.intel.irsa_example;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;

import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ToggleButton;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import android.view.ViewGroup.LayoutParams;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.*;

import android.util.Log;
import android.util.DisplayMetrics;
import android.util.TypedValue;

import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.irsa.*;

public class MainActivity extends Activity {

    private MainActivity gMe;
    private IrsaMgr mIrsaMgr = null;
    private MyEventListener mEventListener = new MyEventListener();


    private static final String TAG = "irsa_example";

    private int screenWidth = 0;
    private int screenHeight = 0;

    private int dispWidth = 176;    // surface view's default width
    private int dispHeight = 144;   //surface view's default height;
    private int rowReserved = 10;   //reserved space in horizontal
    private int colReserved = 50;   //reserved space in vertical
    private int rowSpace = 10;      //space between adjacent items in same row
    private int colSpace = 5;       //space between adjacent items in same col
    private int itemsInRow = 1;     //surface view's counts in the same row
    private int itemsInCol = 1;     //surface view's counts in the same col
    private int textHeight = 50;
    private int textWidth = 400;
    private int TEXT_VIEW_SIZE = 20;

    //hardcode color stream format here
    private int PREVIEW_WIDTH = 640;
    private int PREVIEW_HEIGHT = 480;
    private int PREVIEW_FPS = 30;

    private RelativeLayout layout;
    private RadioGroup groupVR = null;
    private RadioButton radioDepth, radioColor, radioIR;
    private ToggleButton btnOn, btnPlay;

    private Vector<RelativeLayout.LayoutParams> vectorLP = new Vector<RelativeLayout.LayoutParams>();
    private Map<Integer, SurfaceView> mapSV = new HashMap<Integer, SurfaceView>();
    private Map<Integer, Surface> mapSurfaceMap = new HashMap<Integer, Surface>();
    private Vector<TextView> vectorLPHint = new Vector<TextView>();

    private int mDeviceCounts = 0;

    private Button btnRegister;
    private TextView txtViewFace;

    private boolean bSetup = false;


    protected class MyEventListener implements IrsaEventListener {

        MyEventListener() {
        }


        @Override
        public void onEvent(IrsaEventType eventType, int what, int arg1, int arg2, Object obj) {
            String eventString = "==== got event from native: " + " eventType: " + eventType.getValue() + " (" + what + ":" + arg1 + ":" + arg2 + ":" + (String)obj + " ) ==== \n\n\n";
            IrsaLog.d(TAG, eventString);

            if (eventType.getValue() == IrsaEvent.IRSA_ERROR) {
                if (arg1 == IrsaEvent.IRSA_ERROR_PROBE_RS) {
                    IrsaLog.d(TAG, "IRSA_ERROR_PROBE_RS");
                    gMe.btnOn.setEnabled(false);
                    gMe.txtViewFace.setText(eventString);
                    Toast.makeText(gMe.getApplicationContext(), eventString, Toast.LENGTH_SHORT).show();
                }

                if (arg1 == IrsaEvent.IRSA_ERROR_PREVIEW) {
                    gMe.txtViewFace.setText(eventString);
                }
            }

            if (eventType.getValue() == IrsaEvent.IRSA_INFO) {
                if (arg1 == IrsaEvent.IRSA_INFO_PREVIEW) {
                    gMe.txtViewFace.setText(eventString);
                }
            }

        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        screenWidth = dm.widthPixels;
        screenHeight = dm.heightPixels - 500;
        IrsaLog.d(TAG, "screenWidth: " + screenWidth + " screenHeight:" + screenHeight);

        layout = (RelativeLayout) findViewById(R.id.glView);

        groupVR = (RadioGroup) findViewById(R.id.radiogroupVR);
        radioDepth = (RadioButton) findViewById(R.id.radioDepth);
        radioColor = (RadioButton) findViewById(R.id.radioColor);
        radioIR = (RadioButton) findViewById(R.id.radioIR);
        groupVR.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkId) {
                if (checkId == radioDepth.getId()) {
                    Log.d(TAG, "select " + "depth mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: depth");
                        mIrsaMgr.setRenderID(0);
                    }
                } else if (checkId == radioColor.getId()) {
                    Log.d(TAG, "select " + "color mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: color");
                        mIrsaMgr.setRenderID(1);
                    }
                } else if (checkId == radioIR.getId()) {
                    Log.d(TAG, "select " + "ir mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: ir");
                        mIrsaMgr.setRenderID(2);
                    }
                }
            }

        });
        radioColor.setChecked(true);

        btnOn = (ToggleButton)findViewById(R.id.btnOn);
        btnPlay = (ToggleButton)findViewById(R.id.btnPlay);
        btnOn.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    mIrsaMgr.open();
                    MainActivity.this.btnPlay.setEnabled(true);
                    setup();
                } else {
                    MainActivity.this.btnPlay.setEnabled(false);
                    mIrsaMgr.close();
                }
            }
        });

        btnPlay.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    mIrsaMgr.startPreview();
                    MainActivity.this.btnOn.setEnabled(false);
                } else {
                    mIrsaMgr.stopPreview();
                    MainActivity.this.btnOn.setEnabled(true);
                }
            }
        });


        try {
            mIrsaMgr = new IrsaMgr(mEventListener);
            mDeviceCounts = mIrsaMgr.getDeviceCounts();
            IrsaLog.d(TAG, "device counts " + mDeviceCounts);
        } catch (IrsaException e) {
            String errorMsg = "exception was thrown by the MainActivity:\n" + " " + e.getMessage();
            IrsaLog.d(TAG, "error occured: " + errorMsg);
            showMsgBox(this, errorMsg);

            e.printStackTrace();
        }

        switch (mDeviceCounts) {
            case 1:
            case 2:
                itemsInRow = mDeviceCounts;
                break;
            default:
                itemsInCol = 1;
                break;
        }

        int objIndex = 0;
        for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
            for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                objIndex = colIndex * itemsInRow + rowIndex;
                SurfaceView objSurfaceView = new SurfaceView(this);
                mapSV.put(Integer.valueOf(objIndex), objSurfaceView);
                addSurfaceView(objSurfaceView, colIndex, rowIndex);
                addHint(colIndex, rowIndex);
            }
        }

        gMe = this;
    }


    @Override
    public void onStop() {
        super.onStop();
        IrsaLog.d(TAG, "cleanup native resource");
        if (mIrsaMgr != null) {
            mIrsaMgr.release();
            mIrsaMgr = null;
        }
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }



    private void addSurfaceView(SurfaceView sView, int colIndex, int rowIndex) {
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);

        if (0 == rowIndex) {
            lp.width = PREVIEW_WIDTH;
            lp.height = PREVIEW_HEIGHT;
            lp.leftMargin = rowReserved;
        } else {
            lp.width = PREVIEW_WIDTH;
            lp.height = PREVIEW_HEIGHT;
            lp.leftMargin = PREVIEW_WIDTH + rowReserved + rowSpace * rowIndex;
        }
        lp.topMargin = colReserved / 2 + TEXT_VIEW_SIZE + textHeight * 2 +  (PREVIEW_HEIGHT + colSpace + textHeight) * colIndex;
        vectorLP.add(lp);
        if (sView != null) {
            sView.setLayoutParams(lp);
            layout.addView(sView);
        }
    }

    private void addHint(int colIndex, int rowIndex) {
        int objIndex = colIndex * itemsInRow + rowIndex;
        txtViewFace = new TextView(this);
        txtViewFace.setSingleLine(false);
        txtViewFace.setGravity(Gravity.LEFT);

        String camera_hint = "unknown";
        switch (rowIndex) {
            case 0:
                camera_hint = "depth";
                break;
            case 1:
                camera_hint = "color";
                break;
            case 2:
                camera_hint = "IR";
                break;
        }
        camera_hint = "color";

        String info = "Camera " + colIndex + ":" + camera_hint;
        txtViewFace.setText(info);
        RelativeLayout.LayoutParams lpText = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_TOP);

        lpText.topMargin = colReserved / 2 + (PREVIEW_HEIGHT + colSpace * 2 + textHeight) * colIndex - lpText.height - 30;
        lpText.width = textWidth;
        lpText.height = textHeight * 3;

        if (0 == rowIndex) {
            lpText.leftMargin = rowReserved;
        } else {
            lpText.leftMargin = PREVIEW_WIDTH + dispWidth * (rowIndex - 1) + rowReserved + rowSpace * rowIndex;
        }
        vectorLPHint.add(txtViewFace);
        txtViewFace.setVisibility(View.VISIBLE);
        txtViewFace.setLayoutParams(lpText);
        layout.addView(txtViewFace);
    }


    private void setup() {
        if (bSetup)
            return;

        SurfaceView objSV = null;
        int index = 0;

        int objIndex = 0;
        for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
            for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                objIndex = colIndex * itemsInRow + rowIndex;
                objSV = mapSV.get(Integer.valueOf(objIndex));
                mapSurfaceMap.put(Integer.valueOf(objIndex), objSV.getHolder().getSurface());
            }
        }

        mIrsaMgr.setStreamFormat(0, PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_FPS, 0);
        mIrsaMgr.setPreviewDisplay(mapSurfaceMap);
        bSetup = true;
    }


	private void showMsgBox(Context context, String message) {
        AlertDialog dialog = new AlertDialog.Builder(context).create();
        dialog.setMessage(message);
        dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        });
        dialog.show();
    }
}
