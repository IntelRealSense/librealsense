package com.intel.realsense.devices_presets;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.text.Html;
import android.util.Log;
import android.view.View;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.DeviceList;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "librs presets for devices example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;


    private static int mSelectedItem = 500;
    private RsContext mRsContext;
    private TextView deviceResponseET;
    private String[] mPresets;
    private Resources mResources;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        mPermissionsGranted = true;

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        mPermissionsGranted = true;
        init();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(mPermissionsGranted)
            init();
        else
            Log.e(TAG, "missing permissions");
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    private void init(){
        RsContext.init(this);
        mRsContext = new RsContext();

        TextView message = findViewById(R.id.presets_list_title);
        deviceResponseET = findViewById(R.id.deviceResponse);
        mResources = getResources();
        mPresets = null;
        mPresets = new String[]{};
        try {
            mPresets = mResources.getAssets().list("presets");
        } catch (IOException e) {
            message.setText("No presets found");
            return;
        }

        if(mPresets.length == 0) {
            message.setText("No presets found");
            return;
        }
        message.setText("Select a preset:");
        // setting RadioGroup
        final RadioGroup presets_group = findViewById(R.id.presets_list_items);
        if(presets_group.getChildCount() == 0) {

            // adding items to the list
            for (int i = 0; i < mPresets.length; ++i) {
                RadioButton button = new RadioButton(this);
                button.setId(i);
                button.setText(mPresets[i].substring(0, mPresets[i].lastIndexOf('.'))); // text is w/o the file termination
                button.setTextSize(16);
                button.setTextColor(getResources().getColor(R.color.white));
                button.setChecked(i == mSelectedItem);
                presets_group.addView(button);
            }
        }

        final String[] finalPresets = mPresets;
        presets_group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                mSelectedItem = checkedId;
            }
        });

        // COPY BUTTON
        View copyButton = findViewById(R.id.copy_button);
        copyButton.setOnClickListener(new View.OnClickListener() {
                                        @Override
                                        public void onClick(View v) {
                                            ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                                            ClipData clip = ClipData.newPlainText("devises preset", deviceResponseET.getText());
                                            clipboard.setPrimaryClip(clip);
                                        }
                                    });

        // OK BUTTON
        View okButton = findViewById(R.id.presets_ok_button);
        okButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                RsContext ctx = new RsContext();
                try (DeviceList devices = ctx.queryDevices()) {
                    if (devices.getDeviceCount() == 0) {
                        Toast.makeText(MainActivity.this, "failed to set preset, no device found", Toast.LENGTH_LONG).show();
                        Log.e(TAG, "failed to set preset, no device found");
                    }
                    try (Device device = devices.createDevice(0)) {
                        if (device == null) {
                            Log.e(TAG, "failed to set preset, device not available");
                        }
                        final String item = finalPresets[mSelectedItem];

                        try {
                            InputStream is = mResources.getAssets().open("presets/" + item);
                            byte[] buffer = new byte[is.available()];
                            is.read(buffer);
                            ByteArrayOutputStream baos = new ByteArrayOutputStream();
                            baos.write(buffer);
                            baos.close();
                            is.close();
                            device.loadPresetFromJson(buffer);
                        } catch (IOException e) {
                            Toast.makeText(MainActivity.this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                            Log.e(TAG, "failed to set preset, failed to open preset file, error: " + e.getMessage());
                            return;
                        } catch (Exception e) {
                            Toast.makeText(MainActivity.this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                            Log.e(TAG, "failed to set preset, error: " + e.getMessage());
                            return;
                        }

                        StringBuilder builderForConsole = new StringBuilder();
                        byte[] bytes =  device.serializePresetToJson();

                        String deviceResponse = new String(bytes);

                        builderForConsole.append("DEVICE RESPONSE: ");
                        builderForConsole.append(deviceResponse);
                        builderForConsole.append("<br>");

                        builderForConsole.append("sensors size: ");
                        builderForConsole.append(device.querySensors().size());
                        builderForConsole.append("<br>");

                        Sensor depthSensor =  device.querySensors().get(0);
//                        Sensor colorSensor = device.querySensors().get(1);

                        for (int i = 0; i < Option.values().length; i++)
                        {
                            try
                            {
                                builderForConsole.append(Option.values()[i].name());
                                builderForConsole.append(depthSensor.getValue(Option.values()[i]));
                                builderForConsole.append("<br>");
                            }
                            catch (Exception e)
                            {
                                builderForConsole.append(e.getMessage());
                                builderForConsole.append("<br>");
                            }
                        }
                       
                        deviceResponseET.setText( Html.fromHtml(builderForConsole.toString(),1));
                    }
                }
                Log.i(TAG, "preset set to: " + finalPresets[mSelectedItem]);
            }
        });
    }


}