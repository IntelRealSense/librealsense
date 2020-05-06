package com.intel.realsense.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;

public class ControlsDialog extends DialogFragment {

    private static final String TAG = "librs controls";

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Activity activity = getActivity();

        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.controls_dialog, null);

        View closeButton = fragmentView.findViewById(R.id.controls_close_button);
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismissAllowingStateLoss();
            }
        });

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setView(fragmentView);
        AlertDialog rv = builder.create();
        rv.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        rv.getWindow().setGravity(Gravity.CENTER_HORIZONTAL | Gravity.TOP);
        return rv;
    }
}
