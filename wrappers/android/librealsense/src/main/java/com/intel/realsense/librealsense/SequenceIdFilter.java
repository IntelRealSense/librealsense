package com.intel.realsense.librealsense;

public class SequenceIdFilter extends Filter {

    public SequenceIdFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    public SequenceIdFilter(int selectedId){
        mHandle = nCreate(mQueue.getHandle());
        setValue(Option.SEQUENCE_ID, (float)selectedId);
    }

    private static native long nCreate(long queueHandle);
}
