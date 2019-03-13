using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Device : Base.PooledObject
    {
        internal override void Initialize()
        {
            Info = new CameraInfos(this);
        }

        internal Device(IntPtr ptr) : base(ptr, null)
        {
            this.Initialize();
        }

        internal static T Create<T>(IntPtr ptr) where T : Device
        {
            return Pool.Get<T>(ptr);
        }

        internal static T Create<T>(IntPtr ptr, Base.Deleter deleter) where T : Device
        {
            var dev = Pool.Get<T>(ptr);
            dev.m_instance.Reset(ptr, deleter);
            return dev;
        }

        public class CameraInfos
        {
            readonly Device m_device;
            public CameraInfos(Device device) { m_device = device; }

            public string this[CameraInfo info]
            {
                get
                {
                    object err;
                    if (NativeMethods.rs2_supports_device_info(m_device.Handle, info, out err) > 0)
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_device_info(m_device.Handle, info, out err));
                    return null;
                }
            }
        }

        /// <summary>
        /// retrieve camera specific information, like versions of various internal components
        /// </summary>
        public CameraInfos Info { get; private set; }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <returns>The list of sensors</returns>
        public SensorList QuerySensors()
        {
            object error;
            var ptr = NativeMethods.rs2_query_sensors(Handle, out error);
            return new SensorList(ptr);
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <value>The list of sensors</value>
        public SensorList Sensors
        {
            get
            {
                return QuerySensors();
            }
        }
        
        /// <summary>
        /// Send hardware reset request to the device. The actual reset is asynchronous.
        /// Note: Invalidates all handles to this device.
        /// </summary>
        public void HardwareReset()
        {
                object error;
                NativeMethods.rs2_hardware_reset(Handle, out error);
        }

        #region Extension

        /// <summary>Test if the given device can be extended to the requested extension.</summary>
        /// <param name="extension">The extension to which the device should be tested if it is extendable</param>
        /// <returns>Non-zero value iff the device can be extended to the given extension</returns>
        public bool Is(Extension extension)
        {
            object error;
            return NativeMethods.rs2_is_device_extendable_to(Handle, extension, out error) != 0;
        }

        public T As<T>() where T : Device
        {
            return Device.Create<T>(Handle);
        }
        #endregion
    }
}
