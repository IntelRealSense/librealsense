// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Runtime.InteropServices;

    /// <summary>
    /// List of adjacent devices, sharing the same physical parent composite device
    /// </summary>
    internal sealed class SensorList : Base.Object, IEnumerable<Sensor>, ICollection
    {
        internal SensorList(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_sensor_list)
        {
        }

        /// <inheritdoc/>
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

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <inheritdoc/>
        public void CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            for (int i = 0; i < Count; i++)
            {
                array.SetValue(this[i], i + index);
            }
        }

        /// <summary>
        /// Gets the number of sensors in the list
        /// </summary>
        public int Count
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_sensors_count(Handle, out error);
            }
        }

        public object SyncRoot => this;

        public bool IsSynchronized => false;

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
