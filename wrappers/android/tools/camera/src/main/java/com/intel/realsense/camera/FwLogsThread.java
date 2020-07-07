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
    private String mFwLogsParsingFilePath = "";
    private boolean mIsParsingFileInitialized = false;
    private boolean mAllFlashLogsAlreadyPulled = false;

    @Override
    public void run() {
        mAreFwLogsRequested = true;
        try(RsContext ctx = new RsContext()) {
            try (DeviceList devices = ctx.queryDevices()) {
                if (devices.getDeviceCount() > 0) {
                    // only device 0 is taken care of, as the camera app is supposed to work only with one device
                    try (Device device = devices.createDevice(0)) {
                        if (device != null) {
                            try (final FwLogger fwLoggerDevice = device.as(Extension.FW_LOGGER)) {
                                mFwLoggerDevice = fwLoggerDevice;
                                if (mFwLogsParsingFilePath != "")
                                    mIsParsingFileInitialized = mFwLoggerDevice.initParser(mFwLogsParsingFilePath);
                                while (mAreFwLogsRequested) {
                                    String logReceived = "";
                                    try (FwLogMsg logMsg = mFwLoggerDevice.getFwLog()) {
                                        if (mFwLoggerDevice.getFwLogPullingStatus()) {
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
                                                int logMsgSize = logMsg.getSize();
                                                byte[] buffer = new byte[logMsgSize];
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
    }


    public void stopLogging()
    {
        mAreFwLogsRequested = false;
    }

    public void getFwLogsFromFlash()
    {
        if(!mAllFlashLogsAlreadyPulled){
            try(RsContext ctx = new RsContext()){
                try(DeviceList devices = ctx.queryDevices()) {
                    if (devices.getDeviceCount() > 0) {
                        // only device 0 is taken care of, as the camera app is supposed to work only with one device
                        try (Device device = devices.createDevice(0)) {
                            if(device != null) {
                                try(final FwLogger fwLoggerDevice = device.as(Extension.FW_LOGGER)){
                                    mFwLoggerDevice = fwLoggerDevice;
                                    if (mFwLogsParsingFilePath != "")
                                        mIsParsingFileInitialized = mFwLoggerDevice.initParser(mFwLogsParsingFilePath);
                                    Log.d(TAG, "-------------------flash logs retrieval--------------------");
                                    while (true) {
                                        String logReceived = "";
                                        try (FwLogMsg logMsg = mFwLoggerDevice.getFwLogsFromFlash()) {
                                            if (mFwLoggerDevice.getFwLogPullingStatus()) {
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
                                                    int logMsgSize = logMsg.getSize();
                                                    byte[] buffer = new byte[logMsgSize];
                                                    buffer = logMsg.getData(buffer);
                                                    for (byte b : buffer) {
                                                        logReceived += String.format("%02X", b) + " ";
                                                    }
                                                    Log.d(TAG, logReceived);
                                                }
                                            }
                                            else {
                                                Log.d(TAG, "No more fw logs in flash");
                                                mAllFlashLogsAlreadyPulled = true;
                                                break;
                                            }
                                        }
                                    }
                                    Log.d(TAG, "-------------------flash logs finished--------------------");
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            Log.d(TAG, "Flash logs already pulled");
        }
    }
}
