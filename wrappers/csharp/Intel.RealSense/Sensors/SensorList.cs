using System;
using System.Collections;
using System.Collections.Generic;

namespace Intel.RealSense.Sensors
{
    public class SensorList : IDisposable, IEnumerable<Sensor>
    {
        public int Count => NativeMethods.rs2_get_sensors_count(instance, out var error);

        public Sensor this[int index]
        {
            get
            {
                var ptr = NativeMethods.rs2_create_sensor(instance, index, out var error);
                return new Sensor(ptr);
            }
        }

        private IntPtr instance;

        public SensorList(IntPtr ptr)
        {
            instance = ptr;
        }

        public IEnumerator<Sensor> GetEnumerator()
        {

            int sensorCount = NativeMethods.rs2_get_sensors_count(instance, out var error);
            for (int i = 0; i < sensorCount; i++)
            {
                var ptr = NativeMethods.rs2_create_sensor(instance, i, out error);
                yield return new Sensor(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
            => GetEnumerator();



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
                NativeMethods.rs2_delete_sensor_list(instance);
                instance = IntPtr.Zero;

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
