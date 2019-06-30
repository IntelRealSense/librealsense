package com.intel.realsense.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ProgressBar;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.ProductLine;
import com.intel.realsense.librealsense.ProgressListener;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.UpdateDevice;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class FirmwareUpdateProgressDialog extends DialogFragment {
    private static final String TAG = "librs fwupdate";

    private ProgressBar mProgressBar;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Activity activity = getActivity();
        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.firmware_update_progress, null);

        mProgressBar = fragmentView.findViewById(R.id.firmwareUpdateProgressBar);

        Thread t = new Thread(mFirmwareUpdate);
        t.start();

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setView(fragmentView);
        AlertDialog rv = builder.create();
        rv.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        return rv;
    }

    private int getFwImageId(Device device){
        if(device.supportsInfo(CameraInfo.PRODUCT_LINE)){
            ProductLine pl = ProductLine.valueOf(device.getInfo(CameraInfo.PRODUCT_LINE));
            switch (pl){
                case D400: return R.raw.fw_d4xx;
                case SR300: return R.raw.fw_sr3xx;
            }
        }
        throw new RuntimeException("FW update is not supported for the connected device");
    }

    private static byte[] readFwFile(Context context, int fwResId) throws IOException {
        InputStream in = context.getResources().openRawResource(fwResId);
        int length = in.available();
        ByteBuffer buff = ByteBuffer.allocateDirect(length);
        int len = in.read(buff.array(),0, buff.capacity());
        in.close();
        byte[] rv = new byte[len];
        buff.get(rv, 0, len);
        return buff.array();
    }

    private Runnable mFirmwareUpdate = new Runnable() {
        @Override
        public void run() {
            boolean done = false;
            RsContext ctx = new RsContext();
            try(DeviceList dl = ctx.queryDevices()){
                if(dl.getDeviceCount() == 0)
                    return;
                try(Device d = dl.createDevice(0)){
                    if(d.is(Extension.UPDATE_DEVICE)) {
                        UpdateDevice fwud = d.as(Extension.UPDATE_DEVICE);
                        int fw_image = getFwImageId(d);
                        final byte[] bytes = readFwFile(getActivity(), fw_image);
                        updateFirmware(fwud, bytes);
                        done = true;
                    }
                }
            }catch (Exception e) {
                Log.e(TAG, "firmware update failed, error: " + e.getMessage());
            }finally {
                ((DetachedActivity)getActivity()).onFwUpdateStatus(done);
                dismiss();
            }
        }
    };

    private void updateFirmware(UpdateDevice device, final byte[] bytes) {
        device.update(bytes, new ProgressListener() {
            @Override
            public void onProgress(final float progress) {
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        int perc = (int) (progress * 100);
                        mProgressBar.setProgress(perc);
                    }
                });
            }
        });
        Log.i(TAG, "Firmware update process finished successfully");
    }
}
