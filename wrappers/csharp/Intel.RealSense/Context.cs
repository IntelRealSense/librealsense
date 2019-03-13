using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// default librealsense context class
    /// </summary>
    public class Context : Base.Object
    {
        public static readonly int api_version;
        public string Version
        {
            get
            {
                if (api_version / 10000 == 0) return api_version.ToString();
                return $"{api_version / 10000}.{(api_version % 10000) / 100}.{api_version % 100}";
            }
        }

        static Context()
        {
            object error;
            api_version = NativeMethods.rs2_get_api_version(out error);
        }

        public static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_context(api_version, out error);
        }

        public Context() : base(Create(), NativeMethods.rs2_delete_context)
        {
            object error;
            onDevicesChangedCallback = new rs2_devices_changed_callback(onDevicesChanged);
            NativeMethods.rs2_set_devices_changed_callback(Handle, onDevicesChangedCallback, IntPtr.Zero, out error);
        }

        // Keeps the delegate alive, if we were to assign onDevicesChanged directly, there'll be 
        // no managed reference it, it will be collected and cause a native exception.
        readonly rs2_devices_changed_callback onDevicesChangedCallback;

        public delegate void OnDevicesChangedDelegate(DeviceList removed, DeviceList added);
        /// <summary>
        /// these events will be raised by the context whenever new RealSense device is connected or existing device gets disconnected
        /// </summary>
        public event OnDevicesChangedDelegate OnDevicesChanged;

        private void onDevicesChanged(IntPtr removedList, IntPtr addedList, IntPtr userData)
        {
            var e = OnDevicesChanged;
            if (e != null)
            {
                using (var removed = new DeviceList(removedList))
                using (var added = new DeviceList(addedList))
                    e(removed, added);
            }
        }


        /// <summary>create a static snapshot of all connected devices at the time of the call</summary>
        /// <param name="include_platform_camera">Controls what kind of devices will be returned</param>
        /// <returns>the list of devices</returns>
        /// <remarks>devices in the collection should be disposed</remarks>
        public DeviceList QueryDevices(bool include_platform_camera = false)
        {
            object error;
            var ptr = NativeMethods.rs2_query_devices_ex(Handle,
                include_platform_camera ? 0xff : 0xfe, out error);
            return new DeviceList(ptr);
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
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
        public PlaybackDevice AddDevice(string file) {
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
    }


}
