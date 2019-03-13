using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public class SensorList : Base.Object, IEnumerable<Sensor>
    {

        internal SensorList(IntPtr ptr) : base(ptr, NativeMethods.rs2_delete_sensor_list)
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

        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_sensors_count(Handle, out error);
                return deviceCount;
            }
        }

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
