package com.intel.realsense.sensor_api;

import android.Manifest;
import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class SensorStartStopTest {

    private static final String TAG = "librs_sensor_api_test";

    public ActivityTestRule<MainActivity> mActivityRule =
            new ActivityTestRule(MainActivity.class, true, true);
    private MainActivity mActivity;


    @Rule
    public RuleChain chain = RuleChain.outerRule(mActivityRule).
            around(GrantPermissionRule.grant(Manifest.permission.CAMERA));

    @Before
    public void adjustProfiles() {
        mActivity = mActivityRule.getActivity();
        mActivity.changeProfile(StreamType.DEPTH, -1, 1280, 720,
                StreamFormat.Z16, 30);
    }

    // test aim: check that no exception occurs
    @Test
    public void testStartStopOnce() {
        mActivity = mActivityRule.getActivity();
        mActivity.start();
        try{Thread.sleep(5000);} catch(Exception e){}
        mActivity.stop();
        try{Thread.sleep(2000);} catch(Exception e){}
    }

    @Test
    public void testStartStopMany() {
        mActivity = mActivityRule.getActivity();
        int iterations = 0;
        while (++iterations < 200) {
            Log.i(TAG, "iteration: " + iterations);
            mActivity.start();
            try{Thread.sleep(3000);} catch(Exception e){}
            mActivity.stop();
            try{Thread.sleep(70);} catch(Exception e){}
        }
    }
}
