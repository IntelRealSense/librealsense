﻿// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// default librealsense context class
    /// </summary>
    public class Context : Base.Object
    {
        static Context()
        {
            object error;
            ApiVersion = NativeMethods.rs2_get_api_version(out error);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Context"/> class.
        /// </summary>
        public Context()
            : base(Create(), NativeMethods.rs2_delete_context)
        {
            onDevicesChangedCallback = new rs2_devices_changed_callback(OnDevicesChangedInternal);
        }

        /// <summary>
        /// the version API encoded into integer value "1.9.3" -> 10903
        /// </summary>
        public static readonly int ApiVersion;

        /// <summary>
        /// Gets the API version
        /// </summary>
        public string Version
        {
            get
            {
                if (ApiVersion / 10000 == 0)
                {
                    return ApiVersion.ToString();
                }

                return $"{ApiVersion / 10000}.{(ApiVersion % 10000) / 100}.{ApiVersion % 100}";
            }
        }

        internal static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_context(ApiVersion, out error);
        }

        // Keeps the delegate alive, if we were to assign onDevicesChanged directly, there'll be
        // no managed reference it, it will be collected and cause a native exception.
        private readonly rs2_devices_changed_callback onDevicesChangedCallback;

        /// <summary>
        /// Delegate to register as per-notifications callback
        /// </summary>
        /// <param name="removed">list of removed devices</param>
        /// <param name="added">list of added devices</param>
        public delegate void OnDevicesChangedDelegate(DeviceList removed, DeviceList added);

        private event OnDevicesChangedDelegate OnDevicesChangedEvent;

        private readonly object deviceChangedEventLock = new object();

        /// <summary>
        /// these events will be raised by the context whenever new RealSense device is connected or existing device gets disconnected
        /// </summary>
        public event OnDevicesChangedDelegate OnDevicesChanged
        {
            add
            {
                lock (deviceChangedEventLock)
                {
                    if (OnDevicesChangedEvent == null)
                    {
                        object error;
                        NativeMethods.rs2_set_devices_changed_callback(Handle, onDevicesChangedCallback, IntPtr.Zero, out error);
                    }

                    OnDevicesChangedEvent += value;
                }
            }

            remove
            {
                lock (deviceChangedEventLock)
                {
                    OnDevicesChangedEvent -= value;
                }
            }
        }

        /// <summary>
        /// Create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <param name="include_platform_camera">Controls what kind of devices will be returned</param>
        /// <returns>the list of devices</returns>
        /// <remarks>devices in the collection should be disposed</remarks>
        public DeviceList QueryDevices(bool include_platform_camera = false)
        {
            object error;
            var ptr = NativeMethods.rs2_query_devices_ex(Handle, include_platform_camera ? 0xff : 0xfe, out error);
            return new DeviceList(ptr);
        }

        /// <summary>
        /// Gets a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <value>the list of devices</value>
        public DeviceList Devices
        {
            get
            {
                return QueryDevices();
            }
        }

        /// <summary>Create a new device and add it to the context</summary>
        /// <param name="file">The file from which the device should be created</param>
        /// <returns>a device that plays data from the file</returns>
        public PlaybackDevice AddDevice(string file)
        {
            object error;
            var ptr = NativeMethods.rs2_context_add_device(Handle, file, out error);
            return Device.Create<PlaybackDevice>(ptr);
        }

        /// <summary>Removes a playback device from the context, if exists</summary>
        /// <param name="file">The file name that was used to add the device</param>
        public void RemoveDevice(string file)
        {
            object error;
            NativeMethods.rs2_context_remove_device(Handle, file, out error);
        }

        private void OnDevicesChangedInternal(IntPtr removedList, IntPtr addedList, IntPtr userData)
        {
            var e = OnDevicesChangedEvent;
            if (e != null)
            {
                using (var removed = new DeviceList(removedList))
                using (var added = new DeviceList(addedList))
                {
                    e(removed, added);
                }
            }
        }
    }
}
