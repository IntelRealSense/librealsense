// License: Apache 2.0.
// zhouwg2000@gmail.com
// 1. this example could be built with gradle or Android Studio;
// 2. illustrate how to using IRSA in PoC/product development activities with librealsense for 
//    Android based device;
// 3. study & research how to enable librealsense running on non-rooted Android based device;
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

import android.view.ViewGroup.LayoutParams;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.*;

import android.util.Log;
import android.util.DisplayMetrics;
import android.util.TypedValue;

import android.net.Uri;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.irsa.*;

public class MainActivity extends Activity {

    private MainActivity mThiz;
    private IrsaMgr mIrsaMgr = null;
    private MyEventListener mEventListener = new MyEventListener();


    private static final String TAG = "librs_irsa_example";

    private int screenWidth = 0;
    private int screenHeight = 0;

    private int dispWidth = 176;    // surface view's default width
    private int dispHeight = 144;   //surface view's default height;
    private int rowReserved = 10;   //reserved space in horizontal
    private int colReserved = 50;   //reserved space in vertical
    private int rowSpace = 10;      //space between adjacent items in same row
    private int colSpace = 5;       //space between adjacent items in same col
    private int itemsInRow = 2;     //surface view's counts in the same row
    private int itemsInCol = 1;     //surface view's counts in the same col
    private int textHeight = 60;
    private int textWidth = 600;
    private int TEXT_VIEW_SIZE = 20;

    //hardcode color/depth stream format here
    private int RGB_FRAME_WIDTH = 640;
    private int RGB_FRAME_HEIGHT = 480;
    private int RGB_FPS = 30;

    private int DEPTH_FRAME_WIDTH = 1280;
    private int DEPTH_FRAME_HEIGHT = 720;
    private int DEPTH_FPS = 6;

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

    private Button btnSurface;
    private Button btnEmitter;
    private TextView txtViewFace;
    private boolean  bRooted = false;

    private boolean  bSetup = false;
    private int mDefaultRenderID = IrsaRS2Type.RS2_STREAM_COLOR;
    private int mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FAKE;

    private static final int REQUEST_EXTERNAL_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.WRITE_EXTERNAL_STORAGE" };

    protected class MyEventListener implements IrsaEventListener {

        MyEventListener() {
        }


        @Override
        public void onEvent(IrsaEventType eventType, int what, int arg1, int arg2, Object obj) {
            String eventString = "==== " + (bRooted ? "device rooted" : "device not rooted, ") + (String) obj + "  === \n\n\n";

            if (eventType.getValue() == IrsaEvent.IRSA_ERROR) {
                if (arg1 == IrsaEvent.IRSA_ERROR_PROBE_RS) {
                    //probe again when click button "ON"
                    //IrsaLog.d(TAG, "IRSA_ERROR_PROBE_RS, disable controls");
                    //mThiz.btnOn.setEnabled(false);
                    mThiz.txtViewFace.setText(eventString);
                    Toast.makeText(mThiz.getApplicationContext(), eventString, Toast.LENGTH_SHORT).show();
                }
                if (arg1 == IrsaEvent.IRSA_ERROR_PREVIEW) {
                    mThiz.txtViewFace.setText(eventString);
                    Toast.makeText(mThiz.getApplicationContext(), eventString, Toast.LENGTH_SHORT).show();
                }
            }

            if (eventType.getValue() == IrsaEvent.IRSA_INFO) {
                if (arg1 == IrsaEvent.IRSA_INFO_PREVIEW) {
                    mThiz.txtViewFace.setText(eventString);
                }
            }

        }
    }


    public static void checkStoragePermissions(Activity activity) {
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
        IrsaLog.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }

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

        groupType = (RadioGroup) findViewById(R.id.radiogroupType);
        radioFake = (RadioButton) findViewById(R.id.radioFake);
        radioRS = (RadioButton) findViewById(R.id.radioRS);
        radioFA = (RadioButton) findViewById(R.id.radioFA);


        groupVR.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkId) {
                if (checkId == radioDepth.getId()) {
                    Log.d(TAG, "select " + "depth mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: depth");
                        mIrsaMgr.disablePreview(mDefaultRenderID);
                        mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_DEPTH, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
                        mDefaultRenderID = IrsaRS2Type.RS2_STREAM_DEPTH;
                    }
                } else if (checkId == radioColor.getId()) {
                    Log.d(TAG, "select " + "color mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: color");
                        mIrsaMgr.disablePreview(mDefaultRenderID);
                        mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
                        mDefaultRenderID = IrsaRS2Type.RS2_STREAM_COLOR;
                    }
                } else if (checkId == radioIR.getId()) {
                    Log.d(TAG, "select " + "ir mode");
                    if (mIrsaMgr != null) {
                        txtViewFace.setText("Camera: ir");
                        mIrsaMgr.disablePreview(mDefaultRenderID);
                        mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_INFRARED, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
                        mDefaultRenderID = IrsaRS2Type.RS2_STREAM_INFRARED;
                    }
                }
            }

        });
        radioColor.setChecked(true);


        groupType.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkId) {
                if (checkId == radioFake.getId()) {
                    Log.d(TAG, "select " + "fake preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FAKE;
                } else if (checkId == radioRS.getId()) {
                    Log.d(TAG, "select " + "realsense preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_REALSENSE;
                } else if (checkId == radioFA.getId()) {
                    Log.d(TAG, "select " + "FA preview");
                    mDefaultPreviewType = IrsaRS2Type.IRSA_PREVIEW_FA;
                }
            }

        });
        radioFake.setChecked(true);


        btnOn = (ToggleButton) findViewById(R.id.btnOn);
        btnPlay = (ToggleButton) findViewById(R.id.btnPlay);
        btnOn.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    if (mIrsaMgr != null) {
                        mIrsaMgr.setPreviewType(mDefaultPreviewType);
                        setup();
                        mIrsaMgr.open();
                    }

                    MainActivity.this.btnPlay.setEnabled(true);
                } else {
                    MainActivity.this.btnPlay.setEnabled(false);
                    if (mIrsaMgr != null) {
                        mIrsaMgr.close();
                    }
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
                }
            }
        });

        btnSurface = (Button) findViewById(R.id.toggleButtonSurface);
        btnSurface.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mIrsaMgr != null) {
                    mIrsaMgr.disablePreview(mDefaultRenderID);
                    mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
                }
            }
        });

        btnEmitter = (Button) findViewById(R.id.toggleButtonEmitter);
        btnEmitter.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mIrsaMgr != null) {
                    mIrsaMgr.switchEmitter();
                }
            }
        });


        try {
            mIrsaMgr = new IrsaMgr(mEventListener);
            /**
             * @previewType: IRSA_PREVIEW_FAKE, IRSA_PREVIEW_REALSENSE, IRSA_PREVIEW_FA
             */
            //mIrsaMgr.setPreviewType(IrsaRS2Type.IRSA_PREVIEW_FAKE);
            //mIrsaMgr.setPreviewType(IrsaRS2Type.IRSA_PREVIEW_REALSENSE);
            mIrsaMgr.setPreviewType(mDefaultPreviewType);

            mDeviceCounts = mIrsaMgr.getDeviceCounts();
            IrsaLog.d(TAG, "device counts " + mDeviceCounts);
        } catch (IrsaException e) {
            String errorMsg = "an exception was thrown by the MainActivity:\n" + " " + e.getMessage();
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
        for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
            for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                objIndex = colIndex * itemsInRow + rowIndex;
                SurfaceView objSurfaceView = new SurfaceView(this);
                mapSV.put(Integer.valueOf(objIndex), objSurfaceView);
                addSurfaceView(objSurfaceView, colIndex, rowIndex);
            }
        }
        addHint(0, 0);

        mThiz = this;
        copyData("demo.bag", "/mnt/sdcard/demo.bag");
        getRoot();

        checkStoragePermissions(mThiz);
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

        if (0 == rowIndex) {
            lp.width = RGB_FRAME_WIDTH;
            lp.height = RGB_FRAME_HEIGHT;
            lp.leftMargin = rowReserved;
        } else {
            lp.width = RGB_FRAME_WIDTH;
            lp.height = RGB_FRAME_HEIGHT;
            lp.leftMargin = RGB_FRAME_WIDTH + rowReserved + rowSpace * rowIndex;
        }
        lp.topMargin = colReserved / 2 + TEXT_VIEW_SIZE + textHeight * 2 + (RGB_FRAME_HEIGHT + colSpace + textHeight) * colIndex;
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

        lpText.topMargin = colReserved / 2 + (RGB_FRAME_HEIGHT + colSpace * 2 + textHeight) * colIndex - lpText.height - 30;
        lpText.width = textWidth;
        lpText.height = textHeight * 3;

        if (0 == rowIndex) {
            lpText.leftMargin = rowReserved;
        } else {
            lpText.leftMargin = RGB_FRAME_WIDTH + dispWidth * (rowIndex - 1) + rowReserved + rowSpace * rowIndex;
        }
        vectorLPHint.add(txtViewFace);
        txtViewFace.setVisibility(View.VISIBLE);
        txtViewFace.setLayoutParams(lpText);
        layout.addView(txtViewFace);
    }


    private void setup() {


        SurfaceView objSV = null;
        int index = 0;

        if (mIrsaMgr != null) {
            mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_DEPTH, RGB_FRAME_WIDTH, RGB_FRAME_HEIGHT, RGB_FPS, IrsaRS2Type.RS2_FORMAT_RGB8);
            mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_COLOR, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_Z16);
            mIrsaMgr.setStreamFormat(IrsaRS2Type.RS2_STREAM_INFRARED, DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, DEPTH_FPS, IrsaRS2Type.RS2_FORMAT_RGB8);
        }

        if (!bSetup) {
            int objIndex = 0;
            for (int colIndex = 0; colIndex < itemsInCol; colIndex++) {
                for (int rowIndex = 0; rowIndex < itemsInRow; rowIndex++) {
                    objIndex = colIndex * itemsInRow + rowIndex;
                    objSV = mapSV.get(Integer.valueOf(objIndex));
                    mapSurfaceMap.put(Integer.valueOf(objIndex), objSV.getHolder().getSurface());
                }
            }
            bSetup = true;
        }

        if (mIrsaMgr != null) {
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_DEPTH, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
        }
    }


    private void startPreview() {
        if (mIrsaMgr != null) {
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_DEPTH, mapSV.get(Integer.valueOf(0)).getHolder().getSurface());
            mIrsaMgr.enablePreview(IrsaRS2Type.RS2_STREAM_COLOR, mapSV.get(Integer.valueOf(1)).getHolder().getSurface());
            mIrsaMgr.startPreview();
        }
    }


    private void stopPreview() {
        if (mIrsaMgr != null) {
            mIrsaMgr.disablePreview(IrsaRS2Type.RS2_STREAM_DEPTH);
            mIrsaMgr.disablePreview(IrsaRS2Type.RS2_STREAM_COLOR);
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


    private void getRoot(){
        if (isRooted()){
            bRooted = true;
            Toast.makeText(mThiz, "already rooted", Toast.LENGTH_LONG).show();
        } else {
            bRooted = false;
            Toast.makeText(mThiz, "not rooted", Toast.LENGTH_LONG).show();
            try {
                //ProgressDialog progress_dialog = ProgressDialog.show(mThiz, "ROOT", "try to get root priviledge ...", false, true);
                //Process process = Runtime.getRuntime().exec("su");
            } catch (Exception e){
                Toast.makeText(mThiz, "failed to get root priviledge", Toast.LENGTH_LONG).show();
            }
        }
    }


    private void copyData(String srcAssetName, String dstName) {
        InputStream in = null;
        FileOutputStream out = null;
        File file = new File(dstName);
        if (!file.exists()) {
            try {
                in = this.getAssets().open(srcAssetName);
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
}
