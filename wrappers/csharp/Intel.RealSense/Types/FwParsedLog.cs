// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class FwParsedLog : Base.Object
    {

        internal FwParsedLog(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_fw_log_parsed_message)
        { }

        public static FwParsedLog Create(IntPtr handle)
        {
            return new FwParsedLog(handle);
        }

        public string GetMessage()
        {
            object error;
            var p = NativeMethods.rs2_get_fw_log_parsed_message(Handle, out error);
            return Marshal.PtrToStringAnsi(p);
        }

        public string GetFileName()
        {
            object error;
            var p = NativeMethods.rs2_get_fw_log_parsed_file_name(Handle, out error);
            return Marshal.PtrToStringAnsi(p);
        }

        public string GetThreadName()
        {
            object error;
            var p = NativeMethods.rs2_get_fw_log_parsed_thread_name(Handle, out error);
            return Marshal.PtrToStringAnsi(p);
        }

        public LogSeverity GetSeverity()
        {
            object error;
            return (LogSeverity)NativeMethods.rs2_get_fw_log_parsed_severity(Handle, out error);
        }

        public uint GetLine()
        {
            object error;
            return NativeMethods.rs2_get_fw_log_parsed_line(Handle, out error);
        }

        public uint GetTimestamp()
        {
            object error;
            return NativeMethods.rs2_get_fw_log_parsed_timestamp(Handle, out error);
        }

        public uint GetSequenceId()
        {
            object error;
            return NativeMethods.rs2_get_fw_log_parsed_sequence_id(Handle, out error);
        }

    }
}
