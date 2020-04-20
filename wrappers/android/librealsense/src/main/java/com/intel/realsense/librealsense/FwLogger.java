package com.intel.realsense.librealsense;

public class FwLogger {

    public static void startFwLogging(String filePath) {
        nStartReadingFwLogs(filePath);
    }

    public static void stopFwLogging() {
        nStopReadingFwLogs();
    }

    private static native void nStartReadingFwLogs(String filePath);
    private static native void nStopReadingFwLogs();
}
