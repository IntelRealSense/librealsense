using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class DeviceList : Base.Object, IEnumerable<Device>
    {
        public DeviceList(IntPtr ptr) : base(ptr, NativeMethods.rs2_delete_device_list)
        {
        }

        public IEnumerator<Device> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_device_count(Handle, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_create_device(Handle, i, out error);
                yield return Device.Create<Device>(ptr, NativeMethods.rs2_delete_device);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>Determines number of devices in a list.</summary>
        /// <value>Device count</value>
        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_device_count(Handle, out error);
                return deviceCount;
            }
        }

        /// <summary>Creates a device by index. The device object represents a physical camera and provides the means to manipulate it.</summary>
        /// <param name="index">The zero based index of device to retrieve</param>
        /// <returns>The requested device, should be released by rs2_delete_device</returns>
        public Device this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_create_device(Handle, index, out error);
                return Device.Create<Device>(ptr, NativeMethods.rs2_delete_device);
            }
        }

        /// <summary>Checks if a specific device is contained inside a device list.</summary>
        /// <param name="device">RealSense device to check for</param>
        /// <returns>True if the device is in the list and false otherwise</returns>
        public bool Contains(Device device)
        {
            object error;
            return System.Convert.ToBoolean(NativeMethods.rs2_device_list_contains(Handle, device.Handle, out error));
        }
    }
}
