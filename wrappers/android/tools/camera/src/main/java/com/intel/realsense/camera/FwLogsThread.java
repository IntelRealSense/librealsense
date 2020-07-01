package com.intel.realsense.camera;

import android.util.Log;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.FwLogMsg;
import com.intel.realsense.librealsense.FwLogParsedMsg;
import com.intel.realsense.librealsense.FwLogger;
import com.intel.realsense.librealsense.RsContext;

public class FwLogsThread extends Thread{
    private static final String TAG = "librs_fwLogsThread";
    private FwLogger mFwLoggerDevice;
    private volatile boolean mAreFwLogsRequested;
    private String mFwLogsParsingFilePath;
    private boolean mIsParsingFileInitialized = false;

    @Override
    public void run() {
        mAreFwLogsRequested = true;
        try(RsContext ctx = new RsContext()) {
            try (DeviceList devices = ctx.queryDevices()) {
                if (devices.getDeviceCount() > 0) {
                    try (Device device = devices.createDevice(0)) {
                        if (device != null) {
                            try (final FwLogger fwLoggerDevice = device.as(Extension.FW_LOGGER)) {
                                mFwLoggerDevice = fwLoggerDevice;
                                if (mIsParsingFileInitialized)
                                    mFwLoggerDevice.initParser(mFwLogsParsingFilePath);
                                while (mAreFwLogsRequested) {
                                    String logReceived = "";
                                    try (FwLogMsg logMsg = mFwLoggerDevice.getFwLog()) {
                                        if (logMsg.getSize() > 0) {
                                            if (mIsParsingFileInitialized) {
                                                try (FwLogParsedMsg parsedMsg = mFwLoggerDevice.parseFwLog(logMsg)) {
                                                    logReceived = parsedMsg.getTimestamp() + " - " +
                                                            parsedMsg.getSeverity() + " " +
                                                            parsedMsg.getFileName() + " " +
                                                            parsedMsg.getLine() + " " +
                                                            parsedMsg.getThreadName() + " " +
                                                            parsedMsg.getMessage();
                                                    Log.d(TAG, logReceived);
                                                }
                                            } else {
                                                logReceived = logMsg.getTimestamp() + " " +
                                                        logMsg.getSeverityStr() + " ";
                                                byte[] buffer = new byte[512];
                                                buffer = logMsg.getData(buffer);
                                                for (byte b : buffer) {
                                                    logReceived += String.format("%02X", b) + " ";
                                                }
                                                Log.d(TAG, logReceived);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    public void init(String fw_logging_file_path)
    {
        mFwLogsParsingFilePath = fw_logging_file_path;
        mIsParsingFileInitialized = true;
    }


    public void stopLogging()
    {
        mAreFwLogsRequested = false;
    }

    public void getFlashLogs()
    {
        try(RsContext ctx = new RsContext()){
            try(DeviceList devices = ctx.queryDevices()) {
                if (devices.getDeviceCount() > 0) {
                    try (Device device = devices.createDevice(0)) {
                        if(device != null) {
                            try(final FwLogger fwLoggerDevice = device.as(Extension.FW_LOGGER)){
                                mFwLoggerDevice = fwLoggerDevice;
                                mFwLoggerDevice.initParser(mFwLogsParsingFilePath);
                                int logsRequested = 15;
                                while (--logsRequested > 0) {
                                    String logReceived = "";
                                    Log.d(TAG, "before getFwLog");
                                    try (FwLogMsg logMsg = mFwLoggerDevice.getFlashLog()) {
                                        Log.d(TAG, "after getFwLog");
                                        if (logMsg.getSize() > 0) {
                                            if (mIsParsingFileInitialized) {
                                                try (FwLogParsedMsg parsedMsg = mFwLoggerDevice.parseFwLog(logMsg)) {
                                                    Log.d("remi", "after parseFwLog");
                                                    logReceived = parsedMsg.getTimestamp() + " - " +
                                                            parsedMsg.getSeverity() + " " +
                                                            parsedMsg.getFileName() + " " +
                                                            parsedMsg.getLine() + " " +
                                                            parsedMsg.getThreadName() + " " +
                                                            parsedMsg.getMessage();
                                                    Log.d(TAG, logReceived);
                                                }
                                            } else {
                                                logReceived = logMsg.getTimestamp() + " " +
                                                        logMsg.getSeverityStr() + " ";
                                                byte[] buffer = new byte[512];
                                                buffer = logMsg.getData(buffer);
                                                for (byte b : buffer) {
                                                    logReceived += String.format("%02X", b) + " ";
                                                }
                                                Log.d(TAG, logReceived);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
