package com.intel.realsense.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class PresetsDialog extends DialogFragment {
    private static final String TAG = "librs presets";
    // mSelectedItem initialized with high number so that no preset would be checked when opening presets at the first time
    private static int mSelectedItem = 500;
    private String[] mPresets;
    private Resources mResources;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Activity activity = getActivity();
        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.presets_dialog, null);

        TextView message = fragmentView.findViewById(R.id.presets_list_title);
        mResources = getResources();

        try {
            mPresets = mResources.getAssets().list("presets");
        } catch (IOException e) {
            message.setText("No presets found");
            return null;
        }

        if(mPresets.length == 0) {
            message.setText("No presets found");
            return null;
        }
        message.setText("Select a preset:");

        // setting RadioGroup
        final RadioGroup presets_group = fragmentView.findViewById(R.id.presets_list_items);
        // adding items to the list
        for (int i = 0; i < mPresets.length; ++i) {
            RadioButton button = new RadioButton(activity);
            button.setId(i);
            button.setText(mPresets[i].substring(0, mPresets[i].lastIndexOf('.'))); // text is w/o the file termination
            button.setTextSize(16);
            button.setTextColor(getResources().getColor(R.color.white));
            button.setChecked(i == mSelectedItem);
            presets_group.addView(button);
        }

        final String[] finalPresets = mPresets;
        presets_group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                mSelectedItem = checkedId;
            }
        });
        // OK BUTTON
        View okButton = fragmentView.findViewById(R.id.presets_ok_button);
        okButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                RsContext ctx = new RsContext();
                try (DeviceList devices = ctx.queryDevices()) {
                    if (devices.getDeviceCount() == 0) {
                        Log.e(TAG, "failed to set preset, no device found");
                        dismiss();
                    }
                    try (Device device = devices.createDevice(0)) {
                        if (device == null || !device.isInAdvancedMode()) {
                            Log.e(TAG, "failed to set preset, device not in advanced mode");
                            dismiss();
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
                            Log.e(TAG, "failed to set preset, failed to open preset file, error: " + e.getMessage());
                        } catch (Exception e) {
                            Log.e(TAG, "failed to set preset, error: " + e.getMessage());
                        } finally {
                            dismiss();
                        }
                    }
                }
                Log.i(TAG, "preset set to: " + finalPresets[mSelectedItem]);
                dismissAllowingStateLoss();
            }
        });

        // Cancel BUTTON
        View cancelButton = fragmentView.findViewById(R.id.presets_cancel_button);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismissAllowingStateLoss();
            }
        });

        // BUILD DIALOG
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setView(fragmentView);
        AlertDialog rv = builder.create();
        rv.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        rv.getWindow().setGravity(Gravity.CENTER_HORIZONTAL | Gravity.TOP);
        return rv;
    }
}
