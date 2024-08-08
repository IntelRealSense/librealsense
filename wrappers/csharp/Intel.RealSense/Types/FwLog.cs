// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class FwLog : Base.Object
    {

        internal FwLog(IntPtr fw_log_ptr)
            : base(fw_log_ptr, NativeMethods.rs2_delete_fw_log_message)
        { }

        public static FwLog Create(IntPtr handle)
        {
            return new FwLog(handle);
        }

        public byte[] GetData()
        {
            int size = GetSize();
            object error;
            IntPtr start = NativeMethods.rs2_fw_log_message_data(Handle, out error);
            byte[] managedBytes = new byte[size];
            Marshal.Copy(start, managedBytes, 0, size);
            return managedBytes;
        }

        public int GetSize()
        {
            object error;
            return NativeMethods.rs2_fw_log_message_size(Handle, out error);
        }

        public uint GetTimestamp()
        {
            object error;
            return NativeMethods.rs2_fw_log_message_timestamp(Handle, out error);
        }

        public LogSeverity GetSeverity()
        {
            object error;
            return (LogSeverity)NativeMethods.rs2_fw_log_message_severity(Handle, out error);
        }
    }
}
