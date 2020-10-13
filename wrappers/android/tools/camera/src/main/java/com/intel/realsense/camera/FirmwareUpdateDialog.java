package com.intel.realsense.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.ProductLine;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Updatable;

public class FirmwareUpdateDialog extends DialogFragment {

    private Button mFwUpdateButton;
    private Button mSkipFwUpdateButton;
    private CheckBox mDontAskAgainCheckBox;
    private TextView mMessage;
    private TextView mTitle;

    private boolean mDontShowAgain = false;

    static private String getFirmwareUpdateMessage(Device device){
        final String recFw = device.supportsInfo(CameraInfo.RECOMMENDED_FIRMWARE_VERSION) ?
                device.getInfo(CameraInfo.RECOMMENDED_FIRMWARE_VERSION) : "unknown";
        final String fw = device.getInfo(CameraInfo.FIRMWARE_VERSION);
        return "The firmware of the connected device is: " + fw +
                "\n\nThe recommended firmware for this device is: " + recFw;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Activity activity = getActivity();
        Bundle bundle = getArguments();

        final boolean fwUpdateRequest = bundle == null ? false : bundle.getBoolean(getString(R.string.firmware_update_request), false);
        final boolean fwUpdateRequired = bundle == null ? false : bundle.getBoolean(getString(R.string.firmware_update_required), false);

        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.firmware_update_notification, null);

        mDontAskAgainCheckBox = fragmentView.findViewById(R.id.dontAskFwUpdateCheckBox);
        mFwUpdateButton = fragmentView.findViewById(R.id.startFwUpdateButton);
        mSkipFwUpdateButton = fragmentView.findViewById(R.id.skipFwUpdateButton);
        mMessage = fragmentView.findViewById(R.id.firmwareUpdateMessage);
        mTitle = fragmentView.findViewById(R.id.firmwareUpdateTitle);

        if(fwUpdateRequest){
            mTitle.setText("Firmware update");
            mSkipFwUpdateButton.setText("Cancel");
        }

        final RsContext rsContext = new RsContext();
        try(DeviceList dl = rsContext.queryDevices()){
            if(dl.getDeviceCount() > 0){
                try(Device device = dl.createDevice(0)){
                    if(fwUpdateRequired)
                        mMessage.setText("The current firmware version of the connected device is not supported by this librealsense version, update is required");
                    else
                        mMessage.setText(getFirmwareUpdateMessage(device));
                }
            }
        }

        mFwUpdateButton = fragmentView.findViewById(R.id.startFwUpdateButton);
        mFwUpdateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dismissAllowingStateLoss();
                try(DeviceList dl = rsContext.queryDevices(ProductLine.DEPTH)){
                    try(Device d = dl.createDevice(0)){
                        if(d != null && d.is(Extension.UPDATABLE))
                            d.<Updatable>as(Extension.UPDATABLE).enterUpdateState();
                        else
                            dismissAllowingStateLoss();
                    }
                }
            }
        });

        mSkipFwUpdateButton = fragmentView.findViewById(R.id.skipFwUpdateButton);
        mSkipFwUpdateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dismissAllowingStateLoss();
                if(!fwUpdateRequest){
                    Intent intent = new Intent(activity, PreviewActivity.class);
                    startActivity(intent);
                }
            }
        });

        if(fwUpdateRequired)
            mSkipFwUpdateButton.setVisibility(View.GONE);

        mDontAskAgainCheckBox = fragmentView.findViewById(R.id.dontAskFwUpdateCheckBox);
        mDontAskAgainCheckBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                SharedPreferences sharedPref = activity.getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                mDontShowAgain = !mDontShowAgain;
                editor.putBoolean(getString(R.string.show_update_firmware), !mDontShowAgain);
                editor.commit();
            }
        });
        if(fwUpdateRequest || fwUpdateRequired)
            mDontAskAgainCheckBox.setVisibility(View.GONE);

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setView(fragmentView);
        AlertDialog rv = builder.create();
        rv.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        return rv;
    }
}