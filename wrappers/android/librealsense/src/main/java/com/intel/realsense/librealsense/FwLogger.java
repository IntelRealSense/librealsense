package com.intel.realsense.librealsense;


import android.util.Log;

import java.io.File;
import java.util.Scanner;


public class FwLogger extends Device {

    boolean mAreFwLogsRequested = false;
    boolean mIsParserAvailable = false;
    Runnable mFwLogsPullingThread;

    FwLogger(long handle){
        super(handle);
        mOwner = false;
        mFwLogsPullingThread = new Runnable() {
            @Override
            public void run() {
                while(mAreFwLogsRequested) {
                    String logReceived;
                    FwLogMsg logMsg = new FwLogMsg(nCreateFwLogMsg(mHandle));
                    boolean fw_log_result = getFwLog(logMsg);
                    if(fw_log_result){
                        if(mIsParserAvailable) {
                            FwLogParsedMsg parsedMsg = new FwLogParsedMsg(nCreateFwLogParsedMsg(mHandle));
                            boolean parsing_result = parseFwLog(logMsg, parsedMsg);
                            if (parsing_result) {
                                logReceived = parsedMsg.getTimestamp() + " - " +
                                        parsedMsg.getSeverity() + " " +
                                        parsedMsg.getFileName() + " " +
                                        parsedMsg.getLine() + " " +
                                        parsedMsg.getThreadName() + " " +
                                        parsedMsg.getMessage();
                                Log.d("remi", logReceived);
                            } else {
                                Log.d("remi", "parsing failed!");
                            }
                        } else {
                            logReceived = logMsg.getTimestamp() + " " +
                                    logMsg.getSeverityStr() + " ";
                            byte[] buffer = new byte[512];
                            buffer = logMsg.getData(buffer);
                            for(byte b : buffer) {
                                logReceived += String.format("%02X", b) + " ";
                            }
                        }
                    }
                }
            }
        };
    }

    public void startFwLogging(){
        // turn ON the flag for fw logs pulling
        mAreFwLogsRequested = true;

        // running the thread for pulling fw logs
        mFwLogsPullingThread.run();
    }

    public void stopFwLogging() {
        mAreFwLogsRequested = false;
    }

    public boolean initParser(String xml_path) {
        // checking the input file path for parsing logs
        if (!xml_path.contentEquals("") ) {
            try{
                try (Scanner scanner = new Scanner( new File(xml_path), "UTF-8" )) {
                    String xml_content = scanner.useDelimiter("\\A").next();
                    if (nInitParser(mHandle, xml_content)) {
                        mIsParserAvailable = true;
                    }
                }
            }
            catch(Exception e){
                throw new RuntimeException("path to fw logs xml did not succeed: " + e.getMessage());
            }
        }
        return mIsParserAvailable;
    }

    public boolean getFwLog(FwLogMsg msg){
        return nGetFwLog(mHandle, msg.getHandle());
    }

    public boolean getFlashLog(FwLogMsg msg){
        return nGetFlashLog(mHandle, msg.getHandle());
    }

    public boolean parseFwLog(FwLogMsg msg, FwLogParsedMsg parsedMsg) {
        return nParseFwLog(mHandle, msg.getHandle(), parsedMsg.getHandle());
    }

    private static native long nCreateFwLogMsg(long handle);
    private static native boolean nGetFwLog(long handle, long fw_log_msg_handle);
    private static native boolean nGetFlashLog(long handle, long fw_log_msg_handle);
    private static native long nCreateFwLogParsedMsg(long handle);
    private static native boolean nInitParser(long handle, String xml_content);
    private static native boolean nParseFwLog(long handle, long fw_log_msg_handle, long parsed_msg_handle);


    /*public static void startFwLogging(String filePath) {
        nStartReadingFwLogs(filePath);
    }

    public static void stopFwLogging() {
        nStopReadingFwLogs();
    }

    private static native void nStartReadingFwLogs(String filePath);
    private static native void nStopReadingFwLogs();*/
}
