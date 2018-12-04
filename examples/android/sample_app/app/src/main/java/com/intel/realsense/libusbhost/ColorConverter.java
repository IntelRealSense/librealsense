package com.intel.realsense.libusbhost;

import android.content.Context;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Matrix4f;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicColorMatrix;
import android.renderscript.ScriptIntrinsicResize;
import android.renderscript.Type;
import android.view.Surface;
import android.view.TextureView;

import java.nio.ByteBuffer;

public class ColorConverter implements TextureView.SurfaceTextureListener {
    private final RenderScript mRs;
    private final ConversionType mType;
    private final ScriptIntrinsicColorMatrix mScriptColorMatrix;
    private final DepthHistogram mDepthHistogram;
    private Allocation mAllocationIn;
    private Allocation mAllocationOut;
    private Allocation mAllocationCodec;
    private Surface mOutputSurface;
    private Surface mCodecSurface;
    ScriptIntrinsicResize mScriptResize;

    ColorConverter(Context ctx, ConversionType type, int w, int h) {
        mRs = RenderScript.create(ctx);
        mScriptResize = ScriptIntrinsicResize.create(mRs);
        mScriptColorMatrix = ScriptIntrinsicColorMatrix.create(mRs);
        mScriptColorMatrix.setAdd(0, 0, 0, 1.0f);
        mScriptColorMatrix.setColorMatrix(getColorMatrix(type));
        mDepthHistogram = new DepthHistogram(8 * 1024, mRs);
        mType = type;
        mAllocationIn = Allocation.createTyped(mRs, getType(type, w, h));
        mAllocationCodec = Allocation.createTyped(mRs,
                new Type.Builder(mRs, Element.RGBA_8888(mRs))
                        .setX(w)
                        .setY(h).create(),
                Allocation.USAGE_SCRIPT);
        mScriptResize.setInput(mAllocationCodec);
    }

    public void setCodecSurface(Surface codecSurface){
        /*mCodecSurface=codecSurface;
        mAllocationCodec.setSurface(mCodecSurface);*/
    }
    void process(ByteBuffer buffer) {
        if (mAllocationOut == null)
            return;
        int bytes = mAllocationIn.getBytesSize();
        if (bytes == buffer.capacity()) {
            switch (mType) {
                case RGB:
                    mAllocationCodec.copyFromUnchecked(buffer.array());
                    mScriptColorMatrix.forEach(mAllocationIn, mAllocationCodec);
                    break;
                case IR:
                    mAllocationCodec.copyFromUnchecked(buffer.array());
                    mScriptColorMatrix.forEach(mAllocationIn, mAllocationCodec);
                    break;
                case RGBA:
                    mAllocationCodec.copyFromUnchecked(buffer.array());
                    break;
                case DEPTH:
                    mAllocationIn.copyFromUnchecked(buffer.array());
                    mDepthHistogram.DepthToRGBA(mAllocationIn, mAllocationCodec);
                    break;
            }
            if(mCodecSurface!=null) {
                //mAllocationCodec.ioSend();
            }
            if(mAllocationOut!=null && mAllocationCodec!=null)
            {
                mScriptResize.forEach_bicubic(mAllocationOut);
                mAllocationOut.ioSend();
            }
            return;
        }
        throw new RuntimeException("Buffer size smaller then allocation size");
    }

    private Matrix4f getColorMatrix(ConversionType type) {
        switch (type) {
            case DEPTH:
                return new Matrix4f(new float[]
                        {

                                1f, 0f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                0f, 0f, 0f, 1
                        });
            case RGBA:
                return new Matrix4f(new float[]
                        {
                                0f, 0f, 1f, 0f,
                                0f, 1f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                0f, 0f, 0f, 1
                        });
            case IR:
                return new Matrix4f(new float[]
                        {
                                1f, 0f, 0f, 0f,
                                0f, 1f, 0f, 0f,
                                0f, 0f, 1f, 0f,
                                0f, 0f, 0f, 1
                        });
            case RGB:
                return new Matrix4f(new float[]
                        {
                                0f, 0f, 1f, 0f,
                                0f, 1f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                0f, 0f, 0f, 1f
                        });
            default:
                return null;
        }
    }

    ;

    private Type getType(ConversionType type, int w, int h) {
        switch (type) {
            case RGB:
                // Output Allocation
                // Create a new Pixel Element of type RGBA
                Element elemOUT = Element.createPixel(mRs, Element.DataType.UNSIGNED_8, Element.DataKind.PIXEL_RGB);
                // Create a new (Type).Builder object of type elemOUT
                Type.Builder TypeOUT = new Type.Builder(mRs, elemOUT);
                // create an Allocation
                return TypeOUT.setX(w).setY(h).create();             // will be used as a SurfaceTexture producer
            case RGBA:
                return Type.createXY(mRs, Element.RGBA_8888(mRs), w, h);
            case IR:
                return Type.createXY(mRs, Element.U8(mRs), w, h);
            case DEPTH:
                return Type.createXY(mRs, Element.U16(mRs), w, h);
            default:
                throw new RuntimeException("Unsupported conversion type");
        }
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mAllocationOut = Allocation.createTyped(mRs,
                new Type.Builder(mRs, Element.RGBA_8888(mRs))
                        .setX(width)
                        .setY(height).create(),
                Allocation.USAGE_SCRIPT |Allocation.USAGE_IO_OUTPUT );
        mOutputSurface = new Surface(surface);
        mAllocationOut.setSurface(mOutputSurface);

    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mAllocationOut = null;
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {

    }
}
