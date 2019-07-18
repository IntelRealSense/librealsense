package com.intel.realsense.librealsense;

import android.content.Context;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertNotEquals;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class JniPipelineTests {

    private static final String TAG = "librs pipelineTests";

    @Before
    public void before() throws Exception {
        Context context = InstrumentationRegistry.getTargetContext();
        RsContext.init(context);
    }

    @Test
    public void startStop() throws Exception {

        int iteration = 0;
        while(iteration++ < 100) {
            try (Pipeline p = new Pipeline()) {
                try(Config c = new Config()) {
                    Log.i(TAG, "iteration: " + iteration);
                    c.enableStream(StreamType.DEPTH, 0, 1280, 720, StreamFormat.Z16, 30);
                    c.enableStream(StreamType.COLOR, 0, 960, 540, StreamFormat.YUYV, 30);
                    c.enableStream(StreamType.INFRARED, 1, 1280, 720, StreamFormat.Y8, 30);
                    p.start(c);
                }

                int frame = 0;
                while(frame++ < 30){
                    try(FrameSet f = p.waitForFrames()){}
                }
                p.stop();
            }
        }
    }
}
