// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The device object represents a physical camera and provides the means to manipulate it.
    /// </summary>
    public class Device : Base.PooledObject
    {
        internal override void Initialize()
        {
            Info = new CameraInfos(this);
        }

        internal Device(IntPtr ptr)
            : base(ptr, null)
        {
            this.Initialize();
        }

        internal Device(IntPtr ptr, Base.Deleter deleter)
            : base(ptr, deleter)
        {
            this.Initialize();
        }

        internal static T Create<T>(IntPtr ptr)
            where T : Device
        {
            return ObjectPool.Get<T>(ptr);
        }

        internal static T Create<T>(IntPtr ptr, Base.Deleter deleter)
            where T : Device
        {
            var dev = ObjectPool.Get<T>(ptr);
            dev.m_instance.Reset(ptr, deleter);
            return dev;
        }

        public class CameraInfos : IEnumerable<KeyValuePair<CameraInfo, string>>
        {
            private readonly Device device;

            internal CameraInfos(Device device)
            {
                this.device = device;
            }

            /// <summary>
            /// Retrieve camera specific information, like versions of various internal components.
            /// </summary>
            /// <param name="info">Camera info type to retrieve</param>
            /// <returns>The requested camera info string, in a format specific to the device model</returns>
            public string this[CameraInfo info]
            {
                get
                {
                    object err;
                    if (NativeMethods.rs2_supports_device_info(device.Handle, info, out err) > 0)
                    {
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_device_info(device.Handle, info, out err));
                    }

                    return null;
                }
            }

            private static readonly CameraInfo[] CameraInfoValues = Enum.GetValues(typeof(CameraInfo)) as CameraInfo[];

            /// <inheritdoc/>
            public IEnumerator<KeyValuePair<CameraInfo, string>> GetEnumerator()
            {
                object error;
                foreach (var key in CameraInfoValues) {
                    if (NativeMethods.rs2_supports_device_info(device.Handle, key, out error) > 0)
                    {
                        var value = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_device_info(device.Handle, key, out error));
                        yield return new KeyValuePair<CameraInfo, string>(key, value);
                    }
                }
            }

            /// <inheritdoc/>
            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
        }

        /// <summary>
        /// Gets camera specific information, like versions of various internal components
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
        /// Gets a static snapshot of all connected devices at the time of the call
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

        /// <summary>Test if the given device can be extended to the requested extension.</summary>
        /// <param name="extension">The extension to which the device should be tested if it is extendable</param>
        /// <returns>Non-zero value iff the device can be extended to the given extension</returns>
        public bool Is(Extension extension)
        {
            object error;
            return NativeMethods.rs2_is_device_extendable_to(Handle, extension, out error) != 0;
        }

        public T As<T>()
            where T : Device
        {
            return Device.Create<T>(Handle);
        }
    }
}
