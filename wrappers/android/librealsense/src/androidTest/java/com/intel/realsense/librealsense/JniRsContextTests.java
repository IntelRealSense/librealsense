package com.intel.realsense.librealsense;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertNotEquals;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class JniRsContextTests {
    @Test
    public void createContext() {
        RsContext c = new RsContext();
        long handle = c.getHandle();
        assertNotEquals(0, handle);
    }
}
