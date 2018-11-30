package com.intel.realsense.libusbhost;

import android.content.Context;
import android.graphics.Bitmap;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Matrix4f;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicColorMatrix;
import android.renderscript.Type;
import android.util.Pair;

import java.nio.ByteBuffer;
import java.util.HashMap;

public class ColorConverter {
    private final RenderScript mRs;
    private final ConversionType mType;
    private final ScriptIntrinsicColorMatrix mScriptColorMatrix;
    private final ScriptC_depth mScriptDepth;
    private HashMap<Bitmap, Pair<Allocation, Allocation>> mAllocations;

    ColorConverter(Context ctx, ConversionType type) {
        mRs = RenderScript.create(ctx);
        mAllocations = new HashMap<>();
        mScriptColorMatrix = ScriptIntrinsicColorMatrix.create(mRs);
        mScriptColorMatrix.setAdd(0, 0, 0, 1.0f);
        mScriptColorMatrix.setColorMatrix(getColorMatrix(type));

        mType = type;
        mScriptDepth = new ScriptC_depth(mRs);
    }

    void toBitmap(ByteBuffer buffer, Bitmap bitmap) {
        Pair<Allocation, Allocation> alloc = getAllocations(bitmap);
        if (alloc.first == null) {
            mAllocations.put(bitmap, new Pair<>(Allocation.createTyped(mRs, getType(mType, bitmap)), alloc.second));
            alloc = mAllocations.get(bitmap);
        }
        int bytes = alloc.first.getBytesSize();
        if (bytes == buffer.capacity()) {
            alloc.first.copyFromUnchecked(buffer.array());
            switch (mType) {
                case BGRA2RGBA:
                    mScriptColorMatrix.forEach(alloc.first, alloc.second);
                    break;
                case DEPTH2RGBA:
                    mScriptDepth.forEach_depth2rgba(alloc.first,alloc.second);
                    break;
            }

            alloc.second.copyTo(bitmap);
            return;
        }
        throw new RuntimeException("Buffer size smaller then allocation size");
    }

    private Matrix4f getColorMatrix(ConversionType type) {
        switch (type) {
            case DEPTH2RGBA:
                return new Matrix4f(new float[]
                        {

                                1f, 0f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                0f, 0f, 0f, 1
                        });
            case BGRA2RGBA:
                return new Matrix4f(new float[]
                        {
                                0f, 0f, 1f, 0f,
                                0f, 1f, 0f, 0f,
                                1f, 0f, 0f, 0f,
                                0f, 0f, 0f, 1
                        });
            default:
                return null;
        }
    }

    ;

    private Type getType(ConversionType type, Bitmap bitmap) {
        switch (type) {
            case BGRA2RGBA:
                return Type.createXY(mRs, Element.RGBA_8888(mRs), bitmap.getWidth(), bitmap.getHeight());
            case DEPTH2RGBA:
                return Type.createXY(mRs, Element.U16(mRs), bitmap.getWidth(), bitmap.getHeight());
            default:
                throw new RuntimeException("Unsupported conversion type");
        }
    }

    private Pair<Allocation, Allocation> getAllocations(Bitmap bitmap) {
        Pair<Allocation, Allocation> ret;
        if (mAllocations.containsKey(bitmap)) {
            ret = mAllocations.get(bitmap);
            if (ret.second.getBytesSize() == bitmap.getByteCount()) {
                return ret;
            }
        }
        ret = new Pair<>(null, Allocation.createFromBitmap(mRs, bitmap));
        mAllocations.put(bitmap, ret);
        return ret;
    }

}
