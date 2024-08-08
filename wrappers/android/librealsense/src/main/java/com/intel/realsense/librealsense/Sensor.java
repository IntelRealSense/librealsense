package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class Sensor extends Options {

    Sensor(long h) {
        mHandle = h;
    }

    public List<StreamProfile> getStreamProfiles(){
        long[] streamProfilesHandles = nGetStreamProfiles(mHandle);
        List<StreamProfile> rv = new ArrayList<>();
        for(long h : streamProfilesHandles){
            rv.add(new StreamProfile(h));
        }
        return rv;
    }

    public List<StreamProfile> getActiveStreams(){
        long[] streamProfilesHandles = nGetActiveStreams(mHandle);
        List<StreamProfile> rv = new ArrayList<>();
        for(long h : streamProfilesHandles){
            rv.add(new StreamProfile(h));
        }
        return rv;
    }

    public <T extends Sensor> T as(Extension extension) throws RuntimeException {
        if (this.is(extension)) {
            switch (extension){
                case ROI: return (T) new RoiSensor(mHandle);
                case DEPTH_SENSOR: return (T) new DepthSensor(mHandle);
                case COLOR_SENSOR: return (T) new ColorSensor(mHandle);
                default: throw new RuntimeException("this API version does not support " + extension.name());
            }
        } else{
            throw new RuntimeException("this sensor is not extendable to " + extension.name());
        }
    }

    public boolean is(Extension extension) {
        return nIsSensorExtendableTo(mHandle, extension.value());
    }

    // this method's name is not open() so that the user will call the
    // closeSensor to close the sensor (and not the close() method which has another aim)
    public void openSensor(StreamProfile sp) {
        nOpen(mHandle, sp.getHandle());
    }

    public void openSensor(List<StreamProfile> sp_list) {
        int size = sp_list.size();
        long[] profiles_array = new long[size];
        for (int i = 0; i < size; ++i) {
            profiles_array[i] = sp_list.get(i).getHandle();
        }
        nOpenMultiple(mHandle, profiles_array, sp_list.size());
    }

    public void start(FrameCallback cb) {
        nStart(mHandle, cb);
    }

    public void stop() {
        nStop(mHandle);
    }

    // release resources - from AutoCloseable interface
    @Override
    public void close() {
        if (mOwner)
            nRelease(mHandle);
    }

    // this method's name is not close() because this is already
    // taken by the inherited method from AutoCloseable interface
    public void closeSensor(){
        nClose(mHandle);
    }

    public StreamProfile findProfile(StreamType type, int index, int width, int height, StreamFormat format, int fps) {
        List<StreamProfile> profiles = getStreamProfiles();
        StreamProfile rv = null;

        for (StreamProfile profile : profiles) {
            if (profile.getType().compareTo(type) == 0) {

                if (profile.is(Extension.VIDEO_PROFILE)) {
                    VideoStreamProfile video_stream_profile = profile.as(Extension.VIDEO_PROFILE);

                    // After using the "as" method we can use the new data type
                    //  for additional operations:
                    StreamFormat sf = video_stream_profile.getFormat();
                    int idx = profile.getIndex();
                    int w = video_stream_profile.getWidth();
                    int h = video_stream_profile.getHeight();
                    int frameRate = video_stream_profile.getFrameRate();

                    if ((index == -1 || idx == index) &&
                            w == width && h == height &&
                            frameRate == fps && (sf.compareTo(format) == 0)) {
                        rv = profile;
                        break;
                    }
                }
            }
        }
        return rv;
    }

    private static native long[] nGetStreamProfiles(long handle);
    private static native long[] nGetActiveStreams(long handle);
    private static native void nRelease(long handle);
    private static native boolean nIsSensorExtendableTo(long handle, int extension);
    private static native void nOpen(long handle, long sp);
    private static native void nOpenMultiple(long handle, long[] sp_list, int num_of_profiles);
    private static native void nStart(long handle, FrameCallback callback);
    private static native void nStop(long handle);
    private static native void nClose(long handle);
}
