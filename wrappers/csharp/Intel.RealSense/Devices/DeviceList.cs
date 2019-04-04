// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    [DebuggerTypeProxy(typeof(DeviceListDebugView))]
    [DebuggerDisplay("Count = {Count}")]
    public class DeviceList : Base.Object, IEnumerable<Device>
    {
        internal static readonly Base.Deleter DeviceDeleter = NativeMethods.rs2_delete_device;

        internal sealed class DeviceListDebugView
        {
            private readonly DeviceList dl;

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public Device[] Items
            {
                get
                {
                    Device[] array = new Device[dl.Count];
                    for (int i = 0; i < dl.Count; i++)
                    {
                        array.SetValue(dl[i], i);
                    }

                    return array;
                }
            }

            public DeviceListDebugView(DeviceList optionList)
            {
                if (optionList == null)
                {
                    throw new ArgumentNullException(nameof(optionList));
                }

                dl = optionList;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DeviceList"/> class.
        /// </summary>
        /// <param name="ptr">Native <c>rs2_device_list*</c> pointer</param>
        public DeviceList(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_device_list)
        {
        }

        /// <summary>
        /// Returns an iterable of devices in the list
        /// </summary>
        /// <returns>an iterable of devices in the list</returns>
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

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>Gets the number of devices in a list.</summary>
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
