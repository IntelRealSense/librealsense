package com.intel.realsense.pipe_api;

import android.Manifest;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import com.intel.realsense.librealsense.Config;
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
public class PipeStartStopTest {

    public ActivityTestRule<MainActivity> mActivityRule =
            new ActivityTestRule(MainActivity.class, true, true);
    private MainActivity mActivity;


    @Rule
    public RuleChain chain = RuleChain.outerRule(mActivityRule).
            around(GrantPermissionRule.grant(Manifest.permission.CAMERA));


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
        int iterations = 5;
        while (--iterations > 0) {
            mActivity.start();
            try{Thread.sleep(5000);} catch(Exception e){}
            mActivity.stop();
            try{Thread.sleep(2000);} catch(Exception e){}
        }
    }

    @Test
    public void testDeviceNotNull() {
        ActivityScenario<MainActivity> activityScenario =
                ActivityScenario.launch(MainActivity.class);
        assert(activityScenario != null);
        activityScenario.onActivity(
                activity -> {
                    //assert(activity.getDevice() != null);
                });
    }
}
