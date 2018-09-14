using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Config : IDisposable
    {
        internal HandleRef Instance;

        public Config()
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_config(out var error));
        }

        public void EnableStream(Stream s, int index = -1)
        {
            NativeMethods.rs2_config_enable_stream(Instance.Handle, s, index, 0, 0, Format.Any, 0, out var error);
        }

        public void EnableStream(Stream stream_type, int stream_index, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            NativeMethods.rs2_config_enable_stream(Instance.Handle, stream_type, stream_index, width, height, format, framerate, out var error);
        }

        public void EnableStream(Stream stream_type, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            NativeMethods.rs2_config_enable_stream(Instance.Handle, stream_type, -1, width, height, format, framerate, out var error);
        }

        public void EnableStream(Stream stream_type, Format format, int framerate = 0)
        {
            NativeMethods.rs2_config_enable_stream(Instance.Handle, stream_type, -1, 0, 0, format, framerate, out var error);
        }

        public void EnableStream(Stream stream_type, int stream_index, Format format, int framerate = 0)
        {
            NativeMethods.rs2_config_enable_stream(Instance.Handle, stream_type, stream_index, 0, 0, format, framerate, out var error);
        }

        public void EnableAllStreams()
        {
            NativeMethods.rs2_config_enable_all_stream(Instance.Handle, out var error);
        }

        public void EnableDevice(string serial)
        {
            NativeMethods.rs2_config_enable_device(Instance.Handle, serial, out var error);
        }

        public void EnableDeviceFromFile(string filename)
        {
            NativeMethods.rs2_config_enable_device_from_file(Instance.Handle, filename, out var error);
        }

        public void EnableRecordToFile(string filename)
        {
            NativeMethods.rs2_config_enable_record_to_file(Instance.Handle, filename, out var error);
        }

        public void DisableStream(Stream s, int index = -1)
        {
            NativeMethods.rs2_config_disable_indexed_stream(Instance.Handle, s, index, out var error);
        }

        public void DisableAllStreams()
        {
            NativeMethods.rs2_config_disable_all_streams(Instance.Handle, out var error);
        }

        public bool CanResolve(Pipeline pipe)
        {
            var res = NativeMethods.rs2_config_can_resolve(Instance.Handle, pipe.instance.Handle, out var error);
            return res > 0;
        }

        public PipelineProfile Resolve(Pipeline pipe)
        {
            var res = NativeMethods.rs2_config_resolve(Instance.Handle, pipe.instance.Handle, out var error);
            return new PipelineProfile(res);
        }

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

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                Release();
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Config()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }
        #endregion

        public void Release()
        {
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_config(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}
