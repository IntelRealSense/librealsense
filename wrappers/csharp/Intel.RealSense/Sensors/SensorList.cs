using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public class SensorList : IDisposable, IEnumerable<Sensor>
    {
        IntPtr m_instance;

        public SensorList(IntPtr ptr)
        {
            m_instance = ptr;
        }


        public IEnumerator<Sensor> GetEnumerator()
        {
            object error;

            int sensorCount = NativeMethods.rs2_get_sensors_count(m_instance, out error);
            for (int i = 0; i < sensorCount; i++)
            {
                var ptr = NativeMethods.rs2_create_sensor(m_instance, i, out error);
                yield return new Sensor(ptr);
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
                int deviceCount = NativeMethods.rs2_get_sensors_count(m_instance, out error);
                return deviceCount;
            }
        }

        public Sensor this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_create_sensor(m_instance, index, out error);
                return new Sensor(ptr);
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                NativeMethods.rs2_delete_sensor_list(m_instance);
                m_instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~SensorList()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
