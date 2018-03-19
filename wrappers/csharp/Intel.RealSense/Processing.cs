using System;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public class ProcessingBlock : IDisposable
    {
        internal HandleRef m_instance;

        Sensor.SensorOptions m_options;
        public Sensor.SensorOptions Options
        {
            get
            {
                return m_options = m_options ?? new Sensor.SensorOptions(m_instance.Handle);
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
                Release();
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~ProcessingBlock()
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
                NativeMethods.rs2_delete_processing_block(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }

    public class Colorizer : ProcessingBlock
    {
        public Colorizer()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_colorizer(out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame Colorize(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        FrameQueue queue;
    }

    public class Align : ProcessingBlock
    {
        public Align(Stream align_to)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_align(align_to, out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public FrameSet Process(FrameSet original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrames();
        }

        FrameQueue queue;
    }

    public class DisparityTransform : ProcessingBlock
    {
        public DisparityTransform(bool transform_to_disparity = true)
        {
            object error;
            byte transform_direction = transform_to_disparity ? (byte)1 : (byte)0;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_disparity_transform_block(transform_direction, out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        FrameQueue queue;
    }

    public class DecimationFilter : ProcessingBlock
    {
        public DecimationFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_decimation_filter_block(out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        FrameQueue queue;
    }

    public class SpatialFilter : ProcessingBlock
    {
        public SpatialFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_spatial_filter_block(out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        FrameQueue queue;
    }

    public class TemporalFilter : ProcessingBlock
    {
        public TemporalFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_temporal_filter_block(out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        FrameQueue queue;
    }


    public class PointCloud : ProcessingBlock
    {
        FrameQueue queue;

        public PointCloud()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out error));
            queue = new FrameQueue();
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public Points Calculate(Frame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as Points;
        }
        public void MapTexture(VideoFrame texture)
        {
            object error;
            Options[Option.TextureSource].Value = Convert.ToSingle(texture.Profile.UniqueID);
            NativeMethods.rs2_frame_add_ref(texture.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, texture.m_instance.Handle, out error);
        }
    }

}
