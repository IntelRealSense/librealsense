// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    /// <summary>
    /// List of adjacent devices, sharing the same physical parent composite device
    /// </summary>
    public class SensorList : Base.Object, IEnumerable<Sensor>
    {
        internal SensorList(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_sensor_list)
        {
        }

        public IEnumerator<Sensor> GetEnumerator()
        {
            object error;

            int sensorCount = NativeMethods.rs2_get_sensors_count(Handle, out error);
            for (int i = 0; i < sensorCount; i++)
            {
                var ptr = NativeMethods.rs2_create_sensor(Handle, i, out error);
                yield return Sensor.Create<Sensor>(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>
        /// Gets the number of sensors in the list
        /// </summary>
        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_sensors_count(Handle, out error);
                return deviceCount;
            }
        }

        /// <summary>
        /// Creates sensor by index
        /// </summary>
        /// <param name="index">the zero based index of sensor to retrieve</param>
        /// <returns>the requested sensor</returns>
        public Sensor this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_create_sensor(Handle, index, out error);
                return Sensor.Create<Sensor>(ptr);
            }
        }
    }
}
