// License: Apache 2.0. See LICENSE file in root directory.
//
// Author: weiguo.zhou@intel.com
//         zhouwg2000@intel.com
//
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
//
//
package com.android.irsa_example;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Build;
import android.support.v4.content.ContextCompat;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;

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
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;

import android.view.ViewGroup.LayoutParams;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.*;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.PixelFormat;

import android.util.Log;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.util.AttributeSet;

import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.util.List;
import java.util.ArrayList;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.irsa.*;

public class MainActivity extends Activity {

    private MainActivity gMe;
    private IrsaMgr mIrsaMgr = null;
    private MyEventListener mEventListener = new MyEventListener();

    private static final String TAG = "librs_irsa_example";

    private int screenWidth     = 0;
    private int screenHeight    = 0;

    private int dispWidth       = 176;      // surface view's default width
    private int dispHeight      = 144;      //surface view's default height;
    private int rowReserved     = 10;       //reserved space in horizontal
    private int colReserved     = 50;       //reserved space in vertical
    private int rowSpace        = 10;       //space between adjacent items in same row
    private int colSpace        = 5;        //space between adjacent items in same col
    private int itemsInRow      = 3;        //surface view's counts in the same row
    private int itemsInCol      = 1;        //surface view's counts in the same col
    private int textHeight      = 60;
    private int textWidth       = 600;
    private int TEXT_VIEW_SIZE  = 20;

    //hardcode color stream format here for FA
    private int RGB_FRAME_WIDTH     = 640;
    private int RGB_FRAME_HEIGHT    = 480;
    private int RGB_FPS             = 30;

    private int DEPTH_FRAME_WIDTH   = 1280;
    private int DEPTH_FRAME_HEIGHT  = 720;
    private int DEPTH_FPS           = 6;

    private RelativeLayout layout;
    private RadioGroup groupVR = null;
    private RadioButton radioDepth, radioColor, radioIR;

    private RadioGroup groupType = null;
    private RadioButton radioFake, radioRS, radioFA;

    private ToggleButton btnOn, btnPlay;

    private Vector<RelativeLayout.LayoutParams> vectorLP = new Vector<RelativeLayout.LayoutParams>();
    private Map<Integer, SurfaceView> mapSV = new HashMap<Integer, SurfaceView>();
    private Map<Integer, Surface> mapSurfaceMap = new HashMap<Integer, Surface>();
    private Vector<TextView> vectorLPHint = new Vector<TextView>();

    private int mDeviceCounts = 0;

    private Button btnRegister;
    private Button btnEmitter;
    private boolean bEnableEmitter = false;

    private TextView txtDevice;

    private TextView txtViewFace;
    private TextView txtViewStatus;
    private Vector<TextView> vectorTextView = new Vector<TextView>();

    private boolean  bRooted= false;
    private boolean  bSetup = false;
    private boolean  bIRFA  = false; //status of FA module's initialization
    private int mDefaultRenderID = IrsaRS2Type.RS2_STREAM_COLOR;
    private int mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FAKE;
    private String mBAGFile = "/mnt/sdcard/demo.bag";

    private Button btnDepthROI;
    private EditText txtDepthROIXStart;
    private EditText txtDepthROIXEnd;
    private EditText txtDepthROIYStart;
    private EditText txtDepthROIYEnd;

    private Button btnColorROI;
    private EditText txtColorROIXStart;
    private EditText txtColorROIXEnd;
    private EditText txtColorROIYStart;
    private EditText txtColorROIYEnd;

    private DrawImageView  mDepthIV;
    private DrawImageView  mColorIV;

    private Button btnLaserPower;
    private EditText txtLaserPower;

    Map<Integer, IrsaStreamProfile> mapDepth = new HashMap<Integer, IrsaStreamProfile>();
    Map<Integer, IrsaStreamProfile> mapColor = new HashMap<Integer, IrsaStreamProfile>();
    Map<Integer, IrsaStreamProfile> mapIR    = new HashMap<Integer, IrsaStreamProfile>();
    private Vector<Map<Integer, IrsaStreamProfile>> vectorMap = new Vector< Map<Integer, IrsaStreamProfile> >();

    private Spinner spinnerDepth;
    private Spinner spinnerColor;
    private Spinner spinnerIR;
    private Vector<Spinner> vectorSpinner = new Vector<Spinner>();

    private static final int REQUEST_EXTERNAL_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.WRITE_EXTERNAL_STORAGE" };

    protected class MyEventListener implements IrsaEventListener {

        MyEventListener() {
        }


        @Override
        public void onEvent(IrsaEventType eventType, int what, int arg1, int arg2, Object obj) {
            //String eventString = "==== got event from native: " + " eventType: " + eventType.getValue() + " (" + what + ":" + arg1 + ":" + arg2 + ":" + (String)obj + " ) === \n\n\n";
            String eventString = "==== " + (bRooted ? "device rooted, " : "device not rooted, ") + (String) obj + "  === \n\n\n";

            if (eventType.getValue() == IrsaEvent.IRSA_ERROR) {
                if (arg1 == IrsaEvent.IRSA_ERROR_PROBE_RS) {
                    IrsaLog.d(TAG, "IRSA_ERROR_PROBE_RS, disable controls");
                    gMe.btnOn.setEnabled(false);
                    Toast.makeText(gMe.getApplicationContext(), eventString, Toast.LENGTH_SHORT).show();
                }
                if (arg1 == IrsaEvent.IRSA_ERROR_FA) {
                }
                gMe.txtViewStatus.setText(eventString);
            }

            if (eventType.getValue() == IrsaEvent.IRSA_INFO) {
                if (arg1 == IrsaEvent.IRSA_INFO_PREVIEW) {
                    gMe.txtViewFace.setText(eventString);
                }
                if (arg1 == IrsaEvent.IRSA_INFO_FA) {
                    gMe.txtViewStatus.setText(eventString);
                }
            }

        }
    }


    private static void checkStoragePermissions(Activity activity) {
        try {
            int permission = ContextCompat.checkSelfPermission(activity, "android.permission.WRITE_EXTERNAL_STORAGE");
            if (permission != PackageManager.PERMISSION_GRANTED) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) { //check >= Android 6.0
                    activity.requestPermissions(PERMISSIONS_STORAGE, REQUEST_EXTERNAL_STORAGE);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        IrsaLog.d(TAG, "onCreat");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        screenWidth = dm.widthPixels;
        //screenHeight = dm.heightPixels - 500;
        screenHeight = dm.heightPixels;
        IrsaLog.d(TAG, "screenWidth: " + screenWidth + " screenHeight:" + screenHeight);

        layout = (RelativeLayout) findViewById(R.id.glView);

        txtDevice  = ((TextView)findViewById(R.id.txtDevice));

        groupType = (RadioGroup) findViewById(R.id.radiogroupType);
        radioFake = (RadioButton) findViewById(R.id.radioFake);
        radioRS = (RadioButton) findViewById(R.id.radioRS);
        radioFA = (RadioButton) findViewById(R.id.radioFA);

        spinnerDepth = ((Spinner)findViewById(R.id.spinnerDepth));
        spinnerColor = ((Spinner)findViewById(R.id.spinnerColor));
        spinnerIR    = ((Spinner)findViewById(R.id.spinnerIR));
        vectorSpinner.add(spinnerDepth);
        vectorSpinner.add(spinnerColor);
        vectorSpinner.add(spinnerIR);

        groupType.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkId) {
                if (checkId == radioFake.getId()) {
                    Log.d(TAG, "select " + "fake preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FAKE;

                    spinnerDepth.setEnabled(false);
                    spinnerColor.setEnabled(false);
                    spinnerIR.setEnabled(false);
                    btnEmitter.setEnabled(false);
                    btnRegister.setEnabled(false);
                    btnDepthROI.setEnabled(false);
                    btnLaserPower.setEnabled(false);

                    checkStoragePermissions(gMe);
                    //TODO: select bag file from device
                    copyData("demo.bag", "/mnt/sdcard/demo.bag");
                    mBAGFile = "/mnt/sdcard/demo.bag";
                } else if (checkId == radioRS.getId()) {
                    Log.d(TAG, "select " + "realsense preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_REALSENSE;
                    spinnerDepth.setEnabled(true);
                    spinnerColor.setEnabled(true);
                    spinnerIR.setEnabled(true);
                    btnEmitter.setEnabled(true);
                    btnDepthROI.setEnabled(true);
                    btnLaserPower.setEnabled(true);
                } else if (checkId == radioFA.getId()) {
                    Log.d(TAG, "select " + "FA preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FA;
                    //stream profiles are hardcoded with FA mode
                    //IrsaRS2Type.RS2_STREAM_DEPTH, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_Z16
                    //IrsaRS2Type.RS2_STREAM_COLOR, RGB_FRAME_WIDTH, RGB_FRAME_HEIGHT, RGB_FPS, IrsaRS2Type.RS2_FORMAT_RGB8
                    //IrsaRS2Type.RS2_STREAM_INFRARED, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_RGB8
                    String info = "stream profiles are hardcoded with FA mode:" + " depth:1280 * 720 * 6 * RS2_FORMAT_Z16 " + ", color: 640 * 480 * 30 * RS2_FORMAT_RGB8 " + ",IR: 1280 * 720 * 6 * RS2_FORMAT_RGB8"; 
                    spinnerDepth.setEnabled(false);
                    spinnerColor.setEnabled(false);
                    spinnerIR.setEnabled(false);
                    btnEmitter.setEnabled(true);
                    btnRegister.setEnabled(true);
                    btnDepthROI.setEnabled(true);
                    btnLaserPower.setEnabled(true);
                    //String errorMsg = "FA mode disabled because of IPR policy";
                    //showMsgBox(gMe, errorMsg);
                    if (txtViewStatus != null) {
                        txtViewStatus.setText(info);
                    }
                }
            }

        });
        //radioFake.setChecked(true);

        groupVR = (RadioGroup) findViewById(R.id.radiogroupVR);
        radioColor = (RadioButton) findViewById(R.id.radioColor);
        radioIR = (RadioButton) findViewById(R.id.radioIR);
        groupVR.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkId) {
                if (checkId == radioColor.getId()) {
                    String info =  "render Color stream";
                    if (mIrsaMgr != null) {
                        //txtViewStatus.setText(info);
                        mIrsaMgr.disablePreview(mDefaultRenderID);
                        mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
                        mDefaultRenderID = IrsaRS2Type.RS2_STREAM_COLOR;
                    }
                    if (vectorLPHint.size() >= 2) {
                        vectorLPHint.get(1).setText("Camera 0: Color");
                    }
                } else if (checkId == radioIR.getId()) {
                    String info =  "render IR stream";
                    if (mIrsaMgr != null) {
                        //txtViewStatus.setText(info);
                        mIrsaMgr.disablePreview(mDefaultRenderID);
                        mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_INFRARED, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
                        mDefaultRenderID = IrsaRS2Type.RS2_STREAM_INFRARED;
                    }
                    if (vectorLPHint.size() >= 2) {
                        vectorLPHint.get(1).setText("Camera 0: IR");
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
                    if (txtViewStatus != null) {
                        txtViewStatus.setText("");
                        txtViewFace.setText("");
                    }

                    if (mIrsaMgr != null) {
                        mIrsaMgr.setPreviewType(mDefaultPreviewType);
                        if (IrsaRS2Type.IRSA_PREVIEW_FAKE == mDefaultPreviewType) {
                            mIrsaMgr.setBAGFile(mBAGFile);
                        }
                        mIrsaMgr.open();
                        setup();
                    }
                    MainActivity.this.btnPlay.setEnabled(true);
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
                    MainActivity.this.btnOn.setEnabled(false);

                    startPreview();
                } else {
                    stopPreview();

                    MainActivity.this.btnOn.setEnabled(true);

                    mDepthIV.setPos(0, 0, 1, 1);
                    mDepthIV.draw(new Canvas());
                    mDepthIV.invalidate();

                    mColorIV.setPos(0, 0, 1, 1);
                    mColorIV.draw(new Canvas());
                    mColorIV.invalidate();
                }
            }
        });

        btnRegister = (Button) findViewById(R.id.toggleButtonRegister);
        btnRegister.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mIrsaMgr.switchToRegister();
                mIrsaMgr.disablePreview(mDefaultRenderID);
                mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
            }
        });


        btnEmitter = (Button) findViewById(R.id.toggleButtonEmitter);
        btnEmitter.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //mIrsaMgr.switchEmitter();
                mIrsaMgr.setEmitter(bEnableEmitter);
                bEnableEmitter = !bEnableEmitter;
            }
        });


        btnDepthROI = (Button) findViewById(R.id.toggleButtonDepthROI);
        txtDepthROIXStart = (EditText) findViewById(R.id.txtROIXStart);
        txtDepthROIXEnd   = (EditText) findViewById(R.id.txtROIXEnd);
        txtDepthROIYStart = (EditText) findViewById(R.id.txtROIYStart);
        txtDepthROIYEnd   = (EditText) findViewById(R.id.txtROIYEnd);
        btnDepthROI.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                int xs = Integer.parseInt(txtDepthROIXStart.getText().toString());
                int xe = Integer.parseInt(txtDepthROIXEnd.getText().toString());
                int ys = Integer.parseInt(txtDepthROIYStart.getText().toString());
                int ye = Integer.parseInt(txtDepthROIYEnd.getText().toString());
                if ((xs >= 0) && (xe > 0) && (ys >= 0) && (ye > 0) && (xs < xe) && (xe <= DEPTH_FRAME_WIDTH) && (ys < ye) &&  (ye <= DEPTH_FRAME_HEIGHT)) {
                    mIrsaMgr.setROI(IrsaRS2Type.RS2_STREAM_DEPTH, xs, ys, xe, ye);
                    //for Depth stream, the surfaceview's size is NOT equal with stream's profile 
                    //surfaceview 's size is 640  x 480
                    //stream's profile    is 1280 x 720
                    //mDepthIV.setPos(xs, ys, xe - xs, ye - ys);
                    mDepthIV.setPos(xs / 2, ys / 2 + 60 , (xe - xs) / 2, (ye - ys) / 2);
                    mDepthIV.draw(new Canvas());
                    mDepthIV.invalidate();

                    if (mDefaultRenderID == IrsaRS2Type.RS2_STREAM_INFRARED) {
                        mIrsaMgr.setROI(IrsaRS2Type.RS2_STREAM_INFRARED, xs, ys, xe, ye);
                        mColorIV.setPos(xs / 2, ys / 2 + 60 , (xe - xs) / 2, (ye - ys) / 2);
                        mColorIV.draw(new Canvas());
                        mColorIV.invalidate();
                    }

                    IrsaRect rtROI = mIrsaMgr.getROI(IrsaRS2Type.RS2_STREAM_DEPTH);
                    IrsaLog.d(TAG, "getROI: " + rtROI.min_x + "," + rtROI.max_x + "," + rtROI.min_y + "," + rtROI.max_y);
                } else {
                    showMsgBox(gMe, "ROI range error");
                }
            }
        });


        btnColorROI = (Button) findViewById(R.id.toggleButtonColorROI);
        txtColorROIXStart = (EditText) findViewById(R.id.txtColorROIXStart);
        txtColorROIXEnd   = (EditText) findViewById(R.id.txtColorROIXEnd);
        txtColorROIYStart = (EditText) findViewById(R.id.txtColorROIYStart);
        txtColorROIYEnd   = (EditText) findViewById(R.id.txtColorROIYEnd);
        //color ROI disabled with USB2.0 currently
        btnColorROI.setEnabled(false);
        btnColorROI.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                int xs = Integer.parseInt(txtColorROIXStart.getText().toString());
                int xe = Integer.parseInt(txtColorROIXEnd.getText().toString());
                int ys = Integer.parseInt(txtColorROIYStart.getText().toString());
                int ye = Integer.parseInt(txtColorROIYEnd.getText().toString());
                if ((xs >= 0) && (xe > 0) && (ys >= 0) && (ye > 0) && (xs < xe) && (xe <= RGB_FRAME_WIDTH) && (ys < ye) &&  (ye <= RGB_FRAME_HEIGHT)) {
                    mIrsaMgr.setROI(IrsaRS2Type.RS2_STREAM_COLOR, xs, ys, xe, ye);

                    //for Color stream, the surfaceview's size is equal with stream's profile -- both 640 x 480
                    mColorIV.setPos(xs, ys, xe - xs, ye - ys);
                    mColorIV.draw(new Canvas());
                    mColorIV.invalidate();
                } else {
                    showMsgBox(gMe, "ROI range error");
                }
            }
        });

        btnLaserPower = (Button) findViewById(R.id.toggleButtonLaserPower);
        txtLaserPower = (EditText) findViewById(R.id.txtLaserPower);
        btnLaserPower.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                int value = Integer.parseInt(txtLaserPower.getText().toString());
                mIrsaMgr.setLaserPower(value);
            }
        });


        try {
            mIrsaMgr = new IrsaMgr(mEventListener);

            radioRS.setChecked(true);
            mIrsaMgr.setPreviewType(IrsaRS2Type.IRSA_PREVIEW_REALSENSE);

            mDeviceCounts = mIrsaMgr.getDeviceCounts();
            IrsaLog.d(TAG, "device counts " + mDeviceCounts);
            IrsaLog.d(TAG, "device Name: " + mIrsaMgr.getDeviceName());
            IrsaLog.d(TAG, "device SN: " + mIrsaMgr.getDeviceSN());
            IrsaLog.d(TAG, "device FW: " + mIrsaMgr.getDeviceFW());
            String info = "screenWidth: " + screenWidth + " screenHeight:" + screenHeight;
            info += ", Camera Name: " + mIrsaMgr.getDeviceName() + ",SN:" + mIrsaMgr.getDeviceSN() + ",FW:" + mIrsaMgr.getDeviceFW(); 
            txtDevice.setText(info);

            if (mDeviceCounts > 0) {
                initStreamProfiles();
            } else {
                showMsgBox(this, "can't find camera device, init failed");
            }

        } catch (IrsaException e) {
            String errorMsg = "An Exception was thrown by the MainActivity:\n" + " " + e.getMessage();

            IrsaLog.d(TAG, "error occured: " + errorMsg);
            showMsgBox(this, errorMsg);

            e.printStackTrace();
        }

        switch (mDeviceCounts) {
            case 1:
            case 2:
                break;
            default:
                itemsInCol = 1;
                break;
        }

        int objIndex = 0;
        if (screenWidth >= RGB_FRAME_WIDTH * 3)
            itemsInRow = 3;
        else if (screenWidth >= RGB_FRAME_WIDTH * 2)
            itemsInRow = 2;

        for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
            for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                objIndex = colIndex * itemsInRow + rowIndex;
                SurfaceView objSurfaceView = new SurfaceView(this);
                mapSV.put(Integer.valueOf(objIndex), objSurfaceView);
                addSurfaceView(objSurfaceView, colIndex, rowIndex);
                addHint(colIndex, rowIndex);
            }
        }
        txtViewFace = addStatus("FA Status", 0, 0);
        txtViewStatus = addStatus("Common Status:", 0, 1);

        AssetManager assetManager = getAssets();
        try {
            String[] files = assetManager.list("");
            for (String file : files) {
                System.out.println(file);
                if (file.contains("webkit") || file.contains("images") || file.contains("sounds")) {
                    continue;
                }
                String path = this.getApplicationContext().getFilesDir().getAbsolutePath() + "/" + file;
                copyData(file, path);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (mIrsaMgr != null) {
            bIRFA = mIrsaMgr.initIRFA(this.getApplicationContext().getFilesDir().getAbsolutePath());
            IrsaLog.d(TAG, "IRFA initialize " + (bIRFA ? " ok " : "failed"));
            if (!bIRFA) {
                //radioFA.setEnabled(false);
            }
        }

        //copyData("demo.bag", "/mnt/sdcard/demo.bag");

        gMe = this;
        checkRoot();
        //checkStoragePermissions(gMe);
        IrsaLog.d(TAG, "init done");
    }


    private void initStreamProfiles() {
        mapDepth = mIrsaMgr.getStreamProfiles(IrsaRS2Type.RS2_STREAM_DEPTH);
        mapColor = mIrsaMgr.getStreamProfiles(IrsaRS2Type.RS2_STREAM_COLOR);
        mapIR = mIrsaMgr.getStreamProfiles(IrsaRS2Type.RS2_STREAM_INFRARED);

        vectorMap.add(mapDepth);
        vectorMap.add(mapColor);
        vectorMap.add(mapIR);

        IrsaStreamProfile streamProfile;
        IrsaLog.d(TAG, "mapDepth.size " + mapDepth.size());

        for (int i = 0; i < mapDepth.size(); i++) {
            streamProfile = mapDepth.get(Integer.valueOf(i));
            IrsaLog.d(TAG, "w:" + streamProfile.width + ",h: " + streamProfile.height + ",fps: " + streamProfile.fps + ",format:" + mIrsaMgr.formatToString(streamProfile.format));
        }

        ArrayAdapter<String>  adaString;
        List<String>  listString;
        String stringProfile;
        Map<Integer, IrsaStreamProfile> map;

        for (int j = 0; j < vectorMap.size(); j++) {
            map = vectorMap.get(j);
            IrsaLog.d(TAG, "map.size " + map.size());
            listString = new ArrayList<String>();

            for (int i = 0; i < map.size(); i++) {
                streamProfile = map.get(Integer.valueOf(i));
                stringProfile =  streamProfile.width + " * " + streamProfile.height + " * " + streamProfile.fps + " * " + mIrsaMgr.formatToString(streamProfile.format);
                listString.add(stringProfile);
            }
            adaString = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, listString);

            vectorSpinner.get(j).setAdapter(adaString);
        }


        Spinner spinner;
        for (int j = 0; j < vectorSpinner.size(); j++) {
            spinner = vectorSpinner.get(j);
            spinner.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
                    IrsaLog.d(TAG, "select: pos: " + pos + ", id:" + id);
                    IrsaLog.d(TAG, "select: " + spinnerDepth.getSelectedItem().toString());
                    IrsaLog.d(TAG, "select: " + spinnerColor.getSelectedItem().toString());
                    IrsaLog.d(TAG, "select: " + spinnerIR.getSelectedItem().toString());
                }

                public void onNothingSelected(AdapterView<?> parent) {
                    IrsaLog.d(TAG, "select nothing");
                }
            });
        }
    }


    @Override
    public void onDestroy() {
        IrsaLog.d(TAG, "onDestroy cleanup resource");
        if (mIrsaMgr != null) {
            mIrsaMgr.release();
            mIrsaMgr = null;
        }
        super.onDestroy();
    }


    @Override
    public void onStop() {
        IrsaLog.d(TAG, "onStop, cleanup resource");
        if (mIrsaMgr != null) {
            mIrsaMgr.release();
            mIrsaMgr = null;
        }
        super.onStop();
    }


    @Override
    public void onBackPressed() {
        IrsaLog.d(TAG, "onBackPressed, cleanup resource");
        if (mIrsaMgr != null) {
            mIrsaMgr.release();
            mIrsaMgr = null;
        }
        super.onBackPressed();
    }


    private void addSurfaceView(SurfaceView sView, int colIndex, int rowIndex) {
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);

        lp.width = RGB_FRAME_WIDTH;
        lp.height = RGB_FRAME_HEIGHT;

        if (0 == rowIndex) { 
            lp.leftMargin = rowReserved;
        } else {
            lp.leftMargin = RGB_FRAME_WIDTH * rowIndex + rowReserved + rowSpace * rowIndex;
        }

        lp.topMargin = colReserved / 2 + TEXT_VIEW_SIZE + textHeight * 2 +  (RGB_FRAME_HEIGHT + colSpace + textHeight) * colIndex;
        vectorLP.add(lp);
        if (sView != null) {
            sView.setLayoutParams(lp);
            layout.addView(sView);
        }
    }


    private void addHint(int colIndex, int rowIndex) {
        int objIndex = colIndex * itemsInRow + rowIndex;
        TextView txtView = new TextView(this);
        txtView.setSingleLine(false);
        txtView.setGravity(Gravity.LEFT);

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
        String info = "Camera " + colIndex + ":" + camera_hint;
        txtView.setText(info);
        RelativeLayout.LayoutParams lpText = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_TOP);

        lpText.topMargin = colReserved / 2 + TEXT_VIEW_SIZE + textHeight * 2 +  (RGB_FRAME_HEIGHT + colSpace + textHeight) * colIndex - 40;
        lpText.width = textWidth ;
        lpText.height = 40;

        if (0 == rowIndex) {
            lpText.leftMargin = rowReserved;
        } else {
            lpText.leftMargin = RGB_FRAME_WIDTH * rowIndex + dispWidth * (rowIndex - 1) + rowReserved + rowSpace * rowIndex;
        }
        vectorLPHint.add(txtView);
        txtView.setVisibility(View.VISIBLE);
        txtView.setLayoutParams(lpText);
        layout.addView(txtView);
    }

    private TextView addStatus(String info, int colIndex, int rowIndex) {
        if (rowIndex >= 2 )
            return null;

        int objIndex = colIndex * itemsInRow + rowIndex;
        TextView txtView = new TextView(this);
        txtView.setSingleLine(false);
        //txtView.setMaxLines(4);
        txtView.setGravity(Gravity.LEFT);
        //txtView.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_VIEW_SIZE);

        txtView.setText(info);
        RelativeLayout.LayoutParams lpText = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lpText.addRule(RelativeLayout.ALIGN_PARENT_TOP);

        lpText.topMargin = colReserved / 2 + (RGB_FRAME_HEIGHT + colSpace * 2 + textHeight) * colIndex - lpText.height - 30;
        lpText.width = screenWidth - 20;
        lpText.height = textHeight * 2;

        if (0 == rowIndex) {
            lpText.leftMargin = rowReserved;
        } else {
            //lpText.leftMargin = RGB_FRAME_WIDTH * rowIndex + dispWidth * (rowIndex - 1) + rowReserved + rowSpace * rowIndex;
            lpText.leftMargin = (screenWidth > 1200) ? 1200 : 800;
        }
        vectorLPHint.add(txtView);
        txtView.setVisibility(View.VISIBLE);
        txtView.setLayoutParams(lpText);
        layout.addView(txtView);

        return txtView;
    }



    private void setup() {
        SurfaceView objSV = null;
        int index = 0;

        if (mIrsaMgr != null) {
            if (mDefaultPreviewType != IrsaRS2Type.IRSA_PREVIEW_REALSENSE) {
                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_DEPTH, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_Z16);
                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_COLOR, RGB_FRAME_WIDTH, RGB_FRAME_HEIGHT, RGB_FPS, IrsaRS2Type.RS2_FORMAT_RGB8);
                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_INFRARED, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_RGB8);
            } else {
                String profileDepth = spinnerDepth.getSelectedItem().toString();
                String profileColor = spinnerColor.getSelectedItem().toString();
                String profileIR    = spinnerIR.getSelectedItem().toString();
                IrsaLog.d(TAG, "select depth profile: " + profileDepth);
                IrsaLog.d(TAG, "select color profile: " + profileColor);
                IrsaLog.d(TAG, "select IR    profile: " + profileIR);
                String[] pDepth = profileDepth.split("\\*");
                String[] pColor = profileColor.split("\\*");
                String[] pIR    = profileIR.split("\\*");
                IrsaLog.d(TAG, "depth profile: " + pDepth[0] + "," + pDepth[1] + "," + pDepth[2] + "," + pDepth[3]);
                IrsaLog.d(TAG, "depth profile: " + Integer.parseInt(pDepth[0].trim()) + "," + Integer.parseInt(pDepth[1].trim()) + "," + Integer.parseInt(pDepth[2].trim()) + "," + mIrsaMgr.formatFromString(pDepth[3].trim()));

                IrsaLog.d(TAG, "color profile: " + pColor[0] + "," + pColor[1] + "," + pColor[2] + "," + pColor[3]);
                IrsaLog.d(TAG, "color profile: " + Integer.parseInt(pColor[0].trim()) + "," + Integer.parseInt(pColor[1].trim()) + "," + Integer.parseInt(pColor[2].trim()) + "," + mIrsaMgr.formatFromString(pColor[3].trim()));

                IrsaLog.d(TAG, "IR    profile: " + pIR[0] + "," + pIR[1] + "," + pIR[2] + "," + pIR[3]);
                IrsaLog.d(TAG, "IR    profile: " + Integer.parseInt(pIR[0].trim()) + "," + Integer.parseInt(pIR[1].trim()) + "," + Integer.parseInt(pIR[2].trim()) + "," + mIrsaMgr.formatFromString(pIR[3].trim()));

                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_DEPTH, Integer.parseInt(pDepth[0].trim()), Integer.parseInt(pDepth[1].trim()), Integer.parseInt(pDepth[2].trim()), mIrsaMgr.formatFromString(pDepth[3].trim()));
                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_COLOR, Integer.parseInt(pColor[0].trim()), Integer.parseInt(pColor[1].trim()), Integer.parseInt(pColor[2].trim()), mIrsaMgr.formatFromString(pColor[3].trim()));
                mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_INFRARED, Integer.parseInt(pIR[0].trim()), Integer.parseInt(pIR[1].trim()), Integer.parseInt(pIR[2].trim()), mIrsaMgr.formatFromString(pIR[3].trim()));
            }
        }

        if (!bSetup) {
            int objIndex = 0;
            for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
                for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                    objIndex = colIndex * itemsInRow + rowIndex;
                    objSV = mapSV.get(Integer.valueOf(objIndex));
                    objSV.setZOrderOnTop(false);
                    objSV.getHolder().setFormat(PixelFormat.TRANSPARENT);
                    mapSurfaceMap.put(Integer.valueOf(objIndex), objSV.getHolder().getSurface());
                }
            }
            bSetup = true;
        }


        if (mIrsaMgr != null) {
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_DEPTH, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
            //mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
            //mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_INFRARED, mapSV.get(Integer.valueOf(2)).getHolder().getSurface());
            mIrsaMgr.enablePreview(mDefaultRenderID, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
        }

        mDepthIV = new DrawImageView(this);
        mDepthIV.setLayoutParams(vectorLP.get(0));
        layout.addView(mDepthIV);

        mColorIV = new DrawImageView(this);
        mColorIV.setLayoutParams(vectorLP.get(1));
        layout.addView(mColorIV);

        mDepthIV.setPos(0, 0, 1, 1);
        mDepthIV.draw(new Canvas());

        //color ROI disabled with USB2.0 currently
        //color ROI's default range is full screen
        mColorIV.setPos(0, 0, 639, 479);
        mColorIV.draw(new Canvas());
    }


    private void startPreview() {
        if (mIrsaMgr != null) {
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_DEPTH, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
            //mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
            //mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_INFRARED, mapSV.get(Integer.valueOf(2)).getHolder().getSurface());
            mIrsaMgr.enablePreview(mDefaultRenderID, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
            mIrsaMgr.startPreview();
        }
    }


    private void stopPreview() {
        if (mIrsaMgr != null) {
            mIrsaMgr.disablePreview(IrsaRS2Type.RS2_STREAM_DEPTH);
            //mIrsaMgr.disablePreview(IrsaRS2Type.RS2_STREAM_COLOR);
            //mIrsaMgr.disablePreview(IrsaRS2Type.RS2_STREAM_INFRARED);
            mIrsaMgr.disablePreview(mDefaultRenderID);
            mIrsaMgr.stopPreview();
        }
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


    private void copyData(String src, String dst) {
        InputStream in = null;
        FileOutputStream out = null;
        File file = new File(dst);
        if (!file.exists()) {
            try {
                in = this.getAssets().open(src);
                out = new FileOutputStream(file);
                int length = -1;
                byte[] buf = new byte[1024];
                while ((length = in.read(buf)) != -1)  {
                    out.write(buf, 0, length);
                }
                out.flush();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            finally {
                if (in != null) {
                    try {
                        in.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                if (out != null) {
                    try {
                        out.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    private boolean isRooted() {
        boolean res = false;
        try {
            if ((!new File("/system/bin/su").exists()) &&
                    (!new File("/system/xbin/su").exists())) {
                res = false;
            } else {
                res = true;
            }
            ;
        } catch (Exception e) {
            IrsaLog.w(TAG, "exception occurred");
        }
        return res;
    }


    private void checkRoot() {
        if (isRooted()){
            bRooted = true;
            Toast.makeText(gMe, "already rooted", Toast.LENGTH_LONG).show();
        } else {
            bRooted = false;
            Toast.makeText(gMe, "not rooted", Toast.LENGTH_LONG).show();
            try {
                //ProgressDialog progress_dialog = ProgressDialog.show(gMe, "ROOT", "try to get root priviledge ...", false, true);
                //Process process = Runtime.getRuntime().exec("su");
            } catch (Exception e){
                Toast.makeText(gMe, "failed to get root priviledge", Toast.LENGTH_LONG).show();
            }
        }
    }


    private class DrawImageView extends android.support.v7.widget.AppCompatImageView {
        private int mX = 10, mY = 10, mWidth = 100, mHeight = 100;
      
        public DrawImageView(Context context) {  
            super(context);  
        }  
          
        Paint paint = new Paint();  
        {  
            paint.setAntiAlias(true);  
            paint.setColor(Color.RED);  
            paint.setStyle(Style.STROKE);  
            paint.setStrokeWidth(2.5f);
            paint.setAlpha(100);  
        };  
          

        public void setPos(int x, int y, int width, int height) {
            mX      = x;
            mY      = y;
            mWidth  = width;
            mHeight = height;
        }

        @Override  
        protected void onDraw(Canvas canvas) {  
            super.onDraw(canvas);  
            canvas.drawRect(new Rect(mX, mY, mX + mWidth, mY + mHeight), paint);
        }  
    }  

}
