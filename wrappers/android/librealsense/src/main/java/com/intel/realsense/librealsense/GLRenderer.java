package com.intel.realsense.librealsense;

import android.graphics.Point;
import android.graphics.Rect;
import android.opengl.GLES10;
import android.opengl.GLSurfaceView;
import android.util.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLRenderer implements GLSurfaceView.Renderer, AutoCloseable{

    private final Map<Integer,GLFrame> mFrames = new HashMap<>();
    private int mWindowHeight = 0;
    private int mWindowWidth = 0;
    private float mDeltaX = 0;
    private float mDeltaY = 0;
    private Frame mPointsTexture;
    private boolean mHasColorRbg8 = false;
    private Colorizer mColorizer = new Colorizer();
    private Map<StreamType,Pointcloud> mPointcloud = null;
    private boolean mHasColorizedDepth = false;

    public Map<Integer, Pair<String,Rect>> getRectangles() {
        return calcRectangles();
    }

    private boolean showPoints(){
        return mPointcloud != null;
    }

    private List<FilterInterface> createProcessingPipe(){
        List<FilterInterface> rv = new ArrayList<>();
        if(!mHasColorizedDepth && !showPoints())
            rv.add(mColorizer);
        if(showPoints()){
            if(mHasColorRbg8)
                rv.add(mPointcloud.get(StreamType.COLOR));
            else
                rv.add(mPointcloud.get(StreamType.DEPTH));
        }
        return rv;
    }

    private FrameSet applyFilters(FrameSet frameSet, List<FilterInterface> filters){
        frameSet = frameSet.clone();
        for(FilterInterface f : filters){
            FrameSet newSet = frameSet.applyFilter(f);
            frameSet.close();
            frameSet = newSet;
        }
        return frameSet;
    }

    public void upload(FrameSet frameSet) {
        mHasColorRbg8 = mHasColorizedDepth = false;
        frameSet.foreach(new FrameCallback() {
            @Override
            public void onFrame(Frame f) {
                getTexture(f);
            }
        });

        List<FilterInterface> filters = createProcessingPipe();
        try(FrameSet processed = applyFilters(frameSet, filters)){
            choosePointsTexture(processed);
            processed.foreach(new FrameCallback() {
                @Override
                public void onFrame(Frame f) {
                    addFrame(f);
                    upload(f);
                }
            });
        }
    }

    private void choosePointsTexture(FrameSet frameSet){
        if(!showPoints())
            return;
        if(mHasColorRbg8)
            mPointsTexture = frameSet.first(StreamType.COLOR, StreamFormat.RGB8);
        else{
            try (Frame d = frameSet.first(StreamType.DEPTH, StreamFormat.Z16)) {
                if(d != null)
                    mPointsTexture = mColorizer.process(d);
            }
        }
    }

    private void getTexture(Frame f){
        try(StreamProfile sp = f.getProfile()){
            if(sp.getType() == StreamType.COLOR && sp.getFormat() == StreamFormat.RGB8) {
                mHasColorRbg8 = true;
            }
            if(sp.getType() == StreamType.DEPTH && sp.getFormat() == StreamFormat.RGB8) {
                mHasColorizedDepth = true;
            }
        }
    }

    private void addFrame(Frame f){
        try(StreamProfile sp = f.getProfile()){
            if(!isFormatSupported(sp.getFormat()))
                return;
            int uid = sp.getUniqueId();
            if(!mFrames.containsKey(uid)){
                synchronized (mFrames) {
                    if(f.is(Extension.VIDEO_FRAME) && !showPoints())
                        mFrames.put(uid, new GLVideoFrame());
                    if(f.is(Extension.MOTION_FRAME) && !showPoints())
                        mFrames.put(uid, new GLMotionFrame());
                    if(f.is(Extension.POINTS))
                        mFrames.put(uid, new GLPointsFrame());
                }
            }
        }
    }

    public void upload(Frame f) {
        if(f == null)
            return;

        try(StreamProfile sp = f.getProfile()){
            if(!isFormatSupported(sp.getFormat()))
                return;

            addFrame(f);
            int uid = sp.getUniqueId();

            GLFrame curr = mFrames.get(uid);
            if(curr == null)
                return;
            curr.setFrame(f);

            if(mPointsTexture != null && curr instanceof GLPointsFrame){
                ((GLPointsFrame) curr).setTextureFrame(mPointsTexture);
                mPointsTexture.close();
                mPointsTexture = null;
            }
        }
    }

    public void clear() {
        synchronized (mFrames) {
            for(Map.Entry<Integer,GLFrame> f : mFrames.entrySet())
                f.getValue().close();
            mFrames.clear();
            mDeltaX = 0;
            mDeltaY = 0;
            mPointcloud = null;
            if(mPointsTexture != null) mPointsTexture.close();
            mPointsTexture = null;
        }
    }

    private Map<Integer, Pair<String,Rect>> calcRectangles(){
        Map<Integer, Pair<String,Rect>> rv = new HashMap<>();

        int i = 0;
        for (Map.Entry<Integer, GLFrame> entry : mFrames.entrySet()){
            Point size = mWindowWidth > mWindowHeight ?
                    new Point(mWindowWidth / mFrames.size(), mWindowHeight) :
                    new Point(mWindowWidth, mWindowHeight / mFrames.size());
            Point pos = mWindowWidth > mWindowHeight ?
                    new Point(i++ * size.x, 0) :
                    new Point(0, i++ * size.y);
            rv.put(entry.getKey(), new Pair<>(entry.getValue().getLabel(), new Rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y)));
        }
        return rv;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mWindowWidth = width;
        mWindowHeight = height;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (mFrames) {
            GLES10.glViewport(0, 0, mWindowWidth, mWindowHeight);
            GLES10.glClearColor(0, 0, 0, 1);
            GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT | GLES10.GL_DEPTH_BUFFER_BIT);

            if (mFrames.size() == 0)
                return;

            Map<Integer, Pair<String, Rect>> rects = calcRectangles();

            for(Integer uid : mFrames.keySet()){
                GLFrame fl = mFrames.get(uid);
                Rect r = rects.get(uid).second;
                if(mWindowHeight > mWindowWidth){// TODO: remove, w/a for misaligned labels
                    int newTop = mWindowHeight - r.height() - r.top;
                    r = new Rect(r.left, newTop, r.right, newTop + r.height());
                }
                fl.draw(r);
                if(fl instanceof GLPointsFrame){
                    ((GLPointsFrame)fl).rotate(mDeltaX, mDeltaY);
                    mDeltaX = 0;
                    mDeltaY = 0;
                }
            }
        }
    }

    private boolean isFormatSupported(StreamFormat format) {
        switch (format){
            case RGB8:
            case RGBA8:
            case Y8:
            case MOTION_XYZ32F:
            case XYZ32F: return true;
            default: return false;
        }
    }

    public void onTouchEvent(float dx, float dy) {
        synchronized (mFrames) {
            mDeltaX = dx;
            mDeltaY = dy;
        }
    }

    public void showPointcloud(boolean showPoints) {
        if(showPoints){
            if(mPointcloud != null)
                return;
            mPointcloud = new HashMap<>();
            mPointcloud.put(StreamType.COLOR, new Pointcloud(StreamType.COLOR));
            mPointcloud.put(StreamType.DEPTH, new Pointcloud(StreamType.DEPTH));
        }
        else {
            if(mPointcloud == null)
                return;
            for(Pointcloud pc : mPointcloud.values()){
                pc.close();
            }
            mPointcloud = null;
        }
    }

    @Override
    public void close() {
        clear();
        mColorizer.close();
    }
}