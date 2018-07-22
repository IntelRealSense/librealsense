using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Pipeline : IDisposable
    {
        internal HandleRef m_instance;

        public Pipeline(Context ctx)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.m_instance.Handle, out error));
        }

        public Pipeline()
        {
            var ctx = new Context();
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.m_instance.Handle, out error));
        }

        public PipelineProfile Start()
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start(m_instance.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(Config cfg)
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start_with_config(m_instance.Handle, cfg.m_instance.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public void Stop()
        {
            object error;
            NativeMethods.rs2_pipeline_stop(m_instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000, FramesReleaser releaser = null)
        {
            object error;
            var ptr = NativeMethods.rs2_pipeline_wait_for_frames(m_instance.Handle, timeout_ms, out error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(ptr));
        }

        public bool PollForFrames(out FrameSet result, FramesReleaser releaser = null)
        {
            object error;
            FrameSet fs;
            if (NativeMethods.rs2_pipeline_poll_for_frames(m_instance.Handle, out fs, out error) > 0)
            {
                result = FramesReleaser.ScopedReturn(releaser, fs);
                return true;
            }
            result = null;
            return false;
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
                Release();
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Pipeline()
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

        public void Release()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_pipeline(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }

    public class PipelineProfile : IDisposable
    {
        HandleRef m_instance;

        public PipelineProfile(IntPtr p)
        {
            m_instance = new HandleRef(this, p);
        }

        public Device Device
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_device(m_instance.Handle, out error);
                return new Device(ptr);
            }
        }

        public StreamProfileList Streams
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_streams(m_instance.Handle, out error);
                return new StreamProfileList(ptr);
            }
        }

        public StreamProfile GetStream(Stream s, int index = -1)
        {
            foreach(var x in Streams)
            {
                if (x.Stream == s && (index != -1 ? x.Index == index : true))
                    return x;
            }
            return null;
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
                Release();
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~PipelineProfile()
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

        public void Release()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_pipeline_profile(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }

    public class Config : IDisposable
    {
        internal HandleRef m_instance;

        public Config()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_config(out error));
        }

        public void EnableStream(Stream s, int index = -1)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(m_instance.Handle, s, index, 0, 0, Format.Any, 0, out error);
        }

        public void EnableStream(Stream stream_type, int stream_index, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(m_instance.Handle, stream_type, stream_index, width, height, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(m_instance.Handle, stream_type, -1, width, height, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, Format format, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(m_instance.Handle, stream_type, -1, 0, 0, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, int stream_index, Format format, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(m_instance.Handle, stream_type, stream_index, 0, 0, format, framerate, out error);
        }

        public void EnableAllStreams()
        {
            object error;
            NativeMethods.rs2_config_enable_all_stream(m_instance.Handle, out error);
        }

        public void EnableDevice(string serial)
        {
            object error;
            NativeMethods.rs2_config_enable_device(m_instance.Handle, serial, out error);
        }

        public void EnableDeviceFromFile(string filename)
        {
            object error;
            NativeMethods.rs2_config_enable_device_from_file(m_instance.Handle, filename, out error);
        }

        public void EnableRecordToFile(string filename)
        {
            object error;
            NativeMethods.rs2_config_enable_record_to_file(m_instance.Handle, filename, out error);
        }

        public void DisableStream(Stream s, int index = -1)
        {
            object error;
            NativeMethods.rs2_config_disable_indexed_stream(m_instance.Handle, s, index, out error);
        }

        public void DisableAllStreams()
        {
            object error;
            NativeMethods.rs2_config_disable_all_streams(m_instance.Handle, out error);
        }

        public bool CanResolve(Pipeline pipe)
        {
            object error;
            var res = NativeMethods.rs2_config_can_resolve(m_instance.Handle, pipe.m_instance.Handle, out error);
            return res > 0;
        }

        public PipelineProfile Resolve(Pipeline pipe)
        {
            object error;
            var res = NativeMethods.rs2_config_resolve(m_instance.Handle, pipe.m_instance.Handle, out error);
            return new PipelineProfile(res);
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

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion

        public void Release()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_config(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}
