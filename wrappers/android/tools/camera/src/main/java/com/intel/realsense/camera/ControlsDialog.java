package com.intel.realsense.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;

import java.util.List;

public class ControlsDialog extends DialogFragment {
    // TODO - Ariel - Add more controls
    private static final String TAG = "librs controls";
    private Device mDevice;
    private int numOfControls = 0;

    private class ControlParams {
        private String mName;
        private String[] mValues;
        private int mSelectedOptionIndex;
        private Option mOption;

        public ControlParams() {
            mName = new String();
            mValues = null;
            mSelectedOptionIndex = -1;
            mOption = null;
        }
    }

    private ControlParams[] mControlsParams;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {

        final Activity activity = getActivity();
        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.controls_dialog, null);

        // init device
        try(RsContext ctx = new RsContext()){
            try(DeviceList devices = ctx.queryDevices()) {
                mDevice = devices.createDevice(0);
                if (mDevice == null) {
                    Log.e(TAG, "No device could be found");
                    dismissAllowingStateLoss();
                }
            }
        }

        boolean isEmitterOptionSupported = false;

        // retrieving options' current values
        int indexOfCurrentEmitter = 0;
        List<Sensor> sensors = mDevice.querySensors();
        for (Sensor s : sensors) {
            if (s.supports(Option.EMITTER_ENABLED)) {
                indexOfCurrentEmitter = (int)s.getValue(Option.EMITTER_ENABLED);
                isEmitterOptionSupported = true;
                ++numOfControls;
            }
        }

        if (numOfControls > 0) {
            mControlsParams = new ControlParams[numOfControls];
        }

        int controlIndex = 0;

        // EMITTER OPTION
        if (isEmitterOptionSupported) {
            //retrieve values
            final String emitterDescriptions[] = getOptionDescriptions(Option.EMITTER_ENABLED);
            // prepare params for control
            ControlParams controlParams = new ControlParams();
            controlParams.mName = "Projector";
            controlParams.mValues = emitterDescriptions;
            controlParams.mSelectedOptionIndex = indexOfCurrentEmitter;
            controlParams.mOption = Option.EMITTER_ENABLED;

            // add params to list
            mControlsParams[controlIndex++] = controlParams;
        }

        //set controls list
        ListView controlsList = fragmentView.findViewById(R.id.controls_list);
        CustomAdapter customAdapter = new CustomAdapter();
        controlsList.setAdapter(customAdapter);

        // CLOSE BUTTON
        View closeButton = fragmentView.findViewById(R.id.controls_close_button);
        closeButton.setOnClickListener(new View.OnClickListener() {
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

    private String[] getOptionDescriptions(Option option) {
        String[] optionDescriptions;
        List<Sensor> sensors = mDevice.querySensors();
        for (Sensor s : sensors) {
            if (s.supports(option)) {
                int minValue = (int) s.getMinRange(option);
                int maxValue = (int) s.getMaxRange(option);
                optionDescriptions = new String[maxValue - minValue + 1];
                for (int i = minValue; i <= maxValue; ++i) {
                    optionDescriptions[i] = s.valueDescription(option, i);
                }
                return optionDescriptions;
            }
        }
        return null;
    }

    class CustomAdapter extends BaseAdapter {

        @Override
        public int getCount() {
            return numOfControls;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final Activity activity = getActivity();
            convertView = activity.getLayoutInflater().inflate(R.layout.controls_dialog_row, null);

            // setting TextView
            TextView control_name = convertView.findViewById(R.id.control_name);
            control_name.setText(mControlsParams[position].mName);

            // setting RadioGroup
            final RadioGroup control_options = convertView.findViewById(R.id.control_options_list);
            // adding buttons to the group
            for (int i = 0; i < mControlsParams[position].mValues.length; ++i) {
                RadioButton button = new RadioButton(activity);
                button.setId(i);
                button.setText(mControlsParams[position].mValues[i]);
                button.setTextColor(getResources().getColor(R.color.white));
                button.setChecked(i == mControlsParams[position].mSelectedOptionIndex);
                control_options.addView(button);
            }

            final int positionFinal = position;
            control_options.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    List<Sensor> sensors = mDevice.querySensors();
                    for (Sensor s : sensors) {
                        if (s.supports(mControlsParams[positionFinal].mOption)) {
                            s.setValue(Option.EMITTER_ENABLED, checkedId);
                        }
                    }
                }
            });
            return convertView;
        }
    }
}
