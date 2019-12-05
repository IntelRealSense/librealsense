package com.intel.realsense.librealsense;

public class DebugProtocol extends Device {

    DebugProtocol(long handle) {
        super(handle);
    }

    static public String[] getCommands(String commands_file_path){
        return nGetCommands(commands_file_path);
    }

    public String SendAndReceiveRawData(String commands_file_path, String command){
        byte[] data = nSendAndReceiveData(mHandle, commands_file_path, command);
        return new String(data);
    }

    public byte[] SendAndReceiveRawData(byte[] buffer){
        return nSendAndReceiveRawData(mHandle, buffer);
    }
    
    private static native String[] nGetCommands(String filePath);
    private static native byte[] nSendAndReceiveData(long handle, String filePath, String command);
    private static native byte[] nSendAndReceiveRawData(long handle, byte[] buffer);
}
