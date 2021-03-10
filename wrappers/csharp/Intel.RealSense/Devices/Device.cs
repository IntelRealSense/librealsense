// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The device object represents a physical camera and provides the means to manipulate it.
    /// </summary>
    public class Device : Base.RefCountedPooledObject
    {
        protected static Hashtable refCountTable = new Hashtable();
        protected static readonly object tableLock = new object();
        
        internal override void Initialize()
        {
            lock (tableLock)
            {
                if (refCountTable.Contains(Handle))
                    refCount = refCountTable[Handle] as Base.RefCount;
                else
                {
                    refCount = new Base.RefCount();
                    refCountTable[Handle] = refCount;
                }
                Retain();
            }
            Info = new InfoCollection(NativeMethods.rs2_supports_device_info, NativeMethods.rs2_get_device_info, Handle);
        }

        protected override void Dispose(bool disposing)
        {
            if (m_instance.IsInvalid)
            {
                return;
            }

            lock (tableLock)
            {
                IntPtr localHandle = Handle;
                System.Diagnostics.Debug.Assert(refCountTable.Contains(localHandle));

                base.Dispose(disposing);

                if (refCount.count == 0)
                { 
                    refCountTable.Remove(localHandle);
                }
            }
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
            dev.Reset(ptr, deleter);
            return dev;
        }

        /// <summary>
        /// Gets camera specific information, like versions of various internal components
        /// </summary>
        public InfoCollection Info { get; private set; }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <returns>The list of sensors</returns>
        public ReadOnlyCollection<Sensor> QuerySensors()
        {
            object error;
            var ptr = NativeMethods.rs2_query_sensors(Handle, out error);
            using (var sl = new SensorList(ptr))
            {
                var a = new Sensor[sl.Count];
                sl.CopyTo(a, 0);
                return Array.AsReadOnly(a);
            }
        }

        /// <summary>
        /// Gets a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <value>The list of sensors</value>
        public ReadOnlyCollection<Sensor> Sensors => QuerySensors();

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
