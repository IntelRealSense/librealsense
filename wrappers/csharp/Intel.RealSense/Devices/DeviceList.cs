using System;
using System.Collections;
using System.Collections.Generic;

namespace Intel.RealSense.Devices
{
    public class DeviceList : IDisposable, IEnumerable<Device>
    {
        public Device this[int index]
        {
            get
            {
                var ptr = NativeMethods.rs2_create_device(instance, index, out var error);
                return new Device(ptr);
            }
        }
        public int Count
        {
            get
            {
                int deviceCount = NativeMethods.rs2_get_device_count(instance, out var error);
                return deviceCount;
            }
        }

        private IntPtr instance;

        public DeviceList(IntPtr ptr)
        {
            instance = ptr;
        }


        public IEnumerator<Device> GetEnumerator()
        {

            int deviceCount = NativeMethods.rs2_get_device_count(instance, out var error);

            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_create_device(instance, i, out error);
                yield return new Device(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
            => GetEnumerator();



        public bool Contains(Device device)
            => Convert.ToBoolean(NativeMethods.rs2_device_list_contains(instance, device.Instance, out var error));

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }


                NativeMethods.rs2_delete_device_list(instance);
                instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        ~DeviceList()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        #endregion
    }
}
