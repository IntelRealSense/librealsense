// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class TerminalParser : Base.Object
    {
        public TerminalParser(string xml_content)
            : base(Create(xml_content), NativeMethods.rs2_delete_terminal_parser)
        { }


        internal static IntPtr Create(string xml_content)
        {
            object error;
            return NativeMethods.rs2_create_terminal_parser(xml_content, out error);
        }


        public byte[] ParseCommand(string command)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_terminal_parse_command(Handle, command, (uint)command.Length, out error);

            IntPtr start = NativeMethods.rs2_get_raw_data(rawDataBuffer, out error);
            int size = NativeMethods.rs2_get_raw_data_size(rawDataBuffer, out error);

            byte[] managedBytes = new byte[size];
            Marshal.Copy(start, managedBytes, 0, size);
            NativeMethods.rs2_delete_raw_data(rawDataBuffer);

            return managedBytes;
        }

        public string ParseResponse(string command, byte[] response_bytes)
        {
            IntPtr nativeBytes = IntPtr.Zero;
            try
            {
                nativeBytes = Marshal.AllocHGlobal(response_bytes.Length);
                Marshal.Copy(response_bytes, 0, nativeBytes, response_bytes.Length);
                object error;
                IntPtr rawDataBuffer = NativeMethods.rs2_terminal_parse_response(Handle, command, (uint)command.Length, 
                    nativeBytes, (uint)response_bytes.Length, out error);

                IntPtr start = NativeMethods.rs2_get_raw_data(rawDataBuffer, out error);
                int size = NativeMethods.rs2_get_raw_data_size(rawDataBuffer, out error);

                byte[] managedBytes = new byte[size];
                Marshal.Copy(start, managedBytes, 0, size);
                NativeMethods.rs2_delete_raw_data(rawDataBuffer);

                return System.Text.Encoding.Default.GetString(managedBytes);
            }
            finally
            {
                Marshal.FreeHGlobal(nativeBytes);
            }
        }
    }
}
