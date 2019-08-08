package com.intel.realsense.camera;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.Switch;

import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public class StreamProfileAdapter extends ArrayAdapter<StreamProfileSelector> {
    private static final int mLayoutResourceId = R.layout.stream_profile_list_view;
    private final StreamProfileSelector mStreamProfileCells[];
    private final LayoutInflater mLayoutInflater;
    private final Listener mListener;
    private Context mContext;

    public StreamProfileAdapter(Context context, StreamProfileSelector[] data, Listener listener){
        super(context, mLayoutResourceId, data);
        mLayoutInflater = ((Activity) context).getLayoutInflater();
        mStreamProfileCells = data;
        mListener = listener;
        mContext = context;
    }

    public class Holder {
        private Switch type;
        private Spinner index;
        private Spinner resolution;
        private Spinner fps;
        private Spinner format;
    }

    @Override
    public View getView(int position, View rawView, final ViewGroup parent){
        rawView = mLayoutInflater.inflate(mLayoutResourceId, parent, false);
        StreamProfileSelector listViewLine = mStreamProfileCells[position];

        final Holder holder;
        holder = new Holder();
        holder.type = rawView.findViewById(R.id.stream_type_switch);
        holder.resolution = rawView.findViewById(R.id.resolution_spinner);
        holder.fps = rawView.findViewById(R.id.fps_spinner);
        holder.format = rawView.findViewById(R.id.format_spinner);

        holder.type.setText(listViewLine.getName());
        holder.type.setChecked(listViewLine.isEnabled());
        holder.type.setTag(position);

        createSpinners(holder, position, listViewLine);

        return rawView;
    }

    interface Listener{
        void onCheckedChanged(StreamProfileSelector holder);
    }

    void createSpinners(final Holder holder, final int position, StreamProfileSelector sps){
        Set<String> formatsSet = new HashSet<>();
        Set<String> frameRatesSet = new HashSet<>();
        Set<String> resolutionsSet = new HashSet<>();

        for(StreamProfile sp : sps.getProfiles()){
            formatsSet.add(sp.getFormat().name());
            frameRatesSet.add(String.valueOf(sp.getFrameRate()));
            if(!sp.is(Extension.VIDEO_PROFILE))
                continue;
            VideoStreamProfile vsp = sp.as(Extension.VIDEO_PROFILE);
            resolutionsSet.add(String.valueOf(vsp.getWidth()) + "x" + String.valueOf(vsp.getHeight()));
        }

        ArrayList<String> formats = new ArrayList<>(formatsSet);
        ArrayList<String> frameRates = new ArrayList<>(frameRatesSet);
        ArrayList<String> resolutions = new ArrayList<>(resolutionsSet);

        //formats
        ArrayAdapter<String> formatsAdapter = new ArrayAdapter<>(mContext, android.R.layout.simple_spinner_item, formats);
        formatsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        holder.format.setAdapter(formatsAdapter);
        holder.format.setSelection(formats.indexOf(sps.getProfile().getFormat().name()));
        holder.format.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                StreamProfileSelector s = mStreamProfileCells[position];
                String str = (String) adapterView.getItemAtPosition(i);
                s.updateFormat(str);
                mListener.onCheckedChanged(s);
            }
            @Override
            public void onNothingSelected(AdapterView<?> adapterView) { }
        });

        //frame rates
        ArrayAdapter<String> frameRatesAdapter = new ArrayAdapter<>(mContext, android.R.layout.simple_spinner_item, frameRates);
        frameRatesAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        holder.fps.setAdapter(frameRatesAdapter);
        holder.fps.setSelection(frameRates.indexOf(String.valueOf(sps.getProfile().getFrameRate())));
        holder.fps.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                StreamProfileSelector s = mStreamProfileCells[position];
                String str = (String) adapterView.getItemAtPosition(i);
                s.updateFrameRate(str);
                mListener.onCheckedChanged(s);
            }
            @Override
            public void onNothingSelected(AdapterView<?> adapterView) { }
        });

        //resolutions
        ArrayAdapter<String> resolutionsAdapter = new ArrayAdapter<>(mContext, android.R.layout.simple_spinner_item,  resolutions);
        resolutionsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        holder.resolution.setAdapter(resolutionsAdapter);
        holder.resolution.setSelection(resolutions.indexOf(sps.getResolutionString()));
        holder.resolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                StreamProfileSelector s = mStreamProfileCells[position];
                String str = (String) adapterView.getItemAtPosition(i);
                s.updateResolution(str);
                mListener.onCheckedChanged(s);
            }
            @Override
            public void onNothingSelected(AdapterView<?> adapterView) { }
        });

        holder.type.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                int position = (int) compoundButton.getTag();
                StreamProfileSelector s = mStreamProfileCells[position];
                s.updateEnabled(b);
                mListener.onCheckedChanged(s);

            }
        });
    }
}

