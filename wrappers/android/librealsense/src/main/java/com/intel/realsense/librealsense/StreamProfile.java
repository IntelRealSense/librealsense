package com.intel.realsense.librealsense;


public class StreamProfile extends LrsClass {
    private StreamType mType;
    private StreamFormat mFormat;

    private ProfileParams mPp;

    private class ProfileParams {
        public int type;
        public int format;
        public int index;
        public int uniqueId;
        public int frameRate;
    }

    StreamProfile(long handle){
        mHandle = handle;
        mPp = new ProfileParams();
        nGetProfile(mHandle, mPp);
        mType = StreamType.values()[mPp.type];
        mFormat = StreamFormat.values()[mPp.format];
    }

    public StreamType getType() {
        return mType;
    }

    public StreamFormat getFormat() {
        return mFormat;
    }

    public int getIndex() {
        return mPp.index;
    }

    public int getUniqueId() {
        return mPp.uniqueId;
    }

    public int getFrameRate() {
        return mPp.frameRate;
    }

    public Extrinsic getExtrinsicTo(StreamProfile other) throws Exception {
        Extrinsic extrinsic = new Extrinsic();
        nGetExtrinsicTo(mHandle, other.mHandle, extrinsic);
        return extrinsic;
    }

    public void registerExtrinsic(StreamProfile other, Extrinsic extrinsic)
    {
        nRegisterExtrinsic(mHandle, other.mHandle, extrinsic);
    }

    public boolean is(Extension extension) {
        return nIsProfileExtendableTo(mHandle, extension.value());
    }

    public <T extends StreamProfile> T as(Extension extension) {
        switch (extension){
            case VIDEO_PROFILE: return (T) new VideoStreamProfile(mHandle);
            case MOTION_PROFILE: return (T) new MotionStreamProfile(mHandle);
        }
        throw new RuntimeException("this profile is not extendable to " + extension.name());
    }

    @Override
    public void close() {
//        if(mOwner)
//            nDelete(mHandle);
    }

    private static native boolean nIsProfileExtendableTo(long handle, int extension);
    private static native void nGetProfile(long handle, ProfileParams params);
    private static native void nDelete(long handle);
    private static native void nGetExtrinsicTo(long handle, long otherHandle, Extrinsic extrinsic);
    private static native void nRegisterExtrinsic(long handle, long otherHandle, Extrinsic extrinsic);
}
