// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;


    public class SerializableDevice : Device
    {
        internal SerializableDevice(IntPtr dev)
            : base(dev)
        { }

        public static SerializableDevice FromDevice(Device dev)
        {
            return Device.Create<SerializableDevice>(dev.Handle);
        }

        /// <summary>
        /// Gets or sets JSON and applies advanced-mode controls
        /// </summary>
        /// <value>Serialize JSON content</value>
        public string JsonConfiguration
        {
            get
            {
                object error;
                IntPtr buffer = NativeMethods.rs2_serialize_json(Handle, out error);
                int size = NativeMethods.rs2_get_raw_data_size(buffer, out error);
                IntPtr data = NativeMethods.rs2_get_raw_data(buffer, out error);
                var str = Marshal.PtrToStringAnsi(data, size);
                NativeMethods.rs2_delete_raw_data(buffer);
                return str;
            }

            set
            {
                object error;
                NativeMethods.rs2_load_json(Handle, value, (uint)value.Length, out error);
            }
        }
    }
}
