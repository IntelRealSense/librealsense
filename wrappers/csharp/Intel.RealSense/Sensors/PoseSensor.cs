// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class PoseSensor : Sensor
    {
        internal PoseSensor(IntPtr ptr) : base(ptr)
        { }

        public static PoseSensor FromSensor(Sensor sensor)
        {
            if (!sensor.Is(Extension.PoseSensor))
            {
                throw new ArgumentException($"Sensor does not support {nameof(Extension.PoseSensor)}");
            }
            return Create<PoseSensor>(sensor.Handle);
        }

        public byte[] ExportLocalizationMap()
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_export_localization_map(Handle, out error);

            IntPtr start = NativeMethods.rs2_get_raw_data(rawDataBuffer, out error);
            int size = NativeMethods.rs2_get_raw_data_size(rawDataBuffer, out error);

            byte[] managedBytes = new byte[size];
            Marshal.Copy(start, managedBytes, 0, size);
            NativeMethods.rs2_delete_raw_data(rawDataBuffer);

            return managedBytes;
        }

        public bool ImportLocalizationMap(byte[] mapBytes)
        {
            IntPtr nativeBytes = IntPtr.Zero;
            try
            {
                nativeBytes = Marshal.AllocHGlobal(mapBytes.Length);
                Marshal.Copy(mapBytes, 0, nativeBytes, mapBytes.Length);
                object error;
                var res = NativeMethods.rs2_import_localization_map(Handle, nativeBytes, (uint)mapBytes.Length, out error);
                return !(res == 0);
            }
            finally
            {
                Marshal.FreeHGlobal(nativeBytes);
            }
        }

        public bool SetStaticNode(string guid, Math.Vector position, Math.Quaternion rotation)
        {
            object error;
            var res = NativeMethods.rs2_set_static_node(Handle, guid, position, rotation, out error);
            return !(res == 0);
        }

        public bool GetStaticNode(string guid, out Math.Vector position, out Math.Quaternion rotation)
        {
            object error;
            var res = NativeMethods.rs2_get_static_node(Handle, guid, out position, out rotation, out error);
            return !(res == 0);
        }
    }
}
