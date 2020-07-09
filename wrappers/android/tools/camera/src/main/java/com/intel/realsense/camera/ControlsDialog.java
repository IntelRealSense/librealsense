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
import android.widget.RadioButton;
import android.widget.RadioGroup;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;

import java.util.Arrays;
import java.util.List;

public class ControlsDialog extends DialogFragment {
    // TODO - Ariel - Add more controls
    private static final String TAG = "librs controls";

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Activity activity = getActivity();

        LayoutInflater inflater = activity.getLayoutInflater();
        View fragmentView = inflater.inflate(R.layout.controls_dialog, null);

        // getting all existing entries
        final String emitterEntries[] = this.getResources().getStringArray(R.array.controls_emitter_entries);

        int indexOfLastItem = emitterEntries.length - 1;
        int indexOfCurrentEmitter = 0;
        int indexOfCurrentHwPreset = 0;

        RsContext ctx = new RsContext();
        try(DeviceList devices = ctx.queryDevices()) {
            try(Device device = devices.createDevice(0)) {
                if (device != null){
                    List<Sensor> sensors = device.querySensors();
                    for (Sensor s : sensors) {
                        if (s.supports(Option.EMITTER_ENABLED)) {
                            indexOfCurrentEmitter = (int)s.getValue(Option.EMITTER_ENABLED);
                            indexOfLastItem = (int)s.getMaxRange(Option.EMITTER_ENABLED);
                        }
                        if (s.supports(Option.HARDWARE_PRESET)) {
                            indexOfCurrentHwPreset = (int)s.getValue(Option.HARDWARE_PRESET);
                        }
                    }
                }
            }
        }

        // getting only the relevant entries for the sensor
        final String emitterRelevantEntries[] = Arrays.asList(emitterEntries).subList(0, indexOfLastItem + 1).toArray(new String[0]);

        final RadioGroup emittersRadioGroup = fragmentView.findViewById(R.id.emitter_options_list);

        // adding buttons to the group
        for(int i = 0; i < emitterRelevantEntries.length; ++i) {
            RadioButton button = new RadioButton(activity);
            button.setId(i);
            button.setText(emitterRelevantEntries[i]);
            button.setTextColor(getResources().getColor(R.color.white));
            button.setChecked(i == indexOfCurrentEmitter);
            emittersRadioGroup.addView(button);
        }

        emittersRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener(){
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                RsContext ctx = new RsContext();
                try(DeviceList devices = ctx.queryDevices()) {
                    try(Device device = devices.createDevice(0)) {
                        if (device == null)
                            return;
                        List<Sensor> sensors = device.querySensors();
                        for (Sensor s : sensors) {
                            if (s.supports(Option.EMITTER_ENABLED)) {
                                s.setValue(Option.EMITTER_ENABLED, checkedId);
                            }
                        }
                    }
                }
            }
        });

        //HW PRESETS
        final RadioGroup hwPresetsRadioGroup = fragmentView.findViewById(R.id.hw_presets_radio_group);
        // getting entries
        final String hwPresetsEntries[] = this.getResources().getStringArray(R.array.controls_hw_presets_entries);
        final int hwPresetsValues[] = this.getResources().getIntArray(R.array.controls_hw_presets_values);
        // adding buttons to the group
        for(int i = 0; i < hwPresetsEntries.length; ++i) {
            RadioButton button = new RadioButton(activity);
            button.setId(hwPresetsValues[i]);
            button.setText(hwPresetsEntries[i]);
            button.setTextColor(getResources().getColor(R.color.white));
            button.setChecked(hwPresetsValues[i] == indexOfCurrentHwPreset);
            hwPresetsRadioGroup.addView(button);
        }

        hwPresetsRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener(){
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                RsContext ctx = new RsContext();
                try(DeviceList devices = ctx.queryDevices()) {
                    try(Device device = devices.createDevice(0)) {
                        if (device == null)
                            return;
                        List<Sensor> sensors = device.querySensors();
                        for (Sensor s : sensors) {
                            if (s.supports(Option.HARDWARE_PRESET)) {
                                s.setValue(Option.HARDWARE_PRESET, checkedId);
                            }
                        }
                    }
                }
            }
        });


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
