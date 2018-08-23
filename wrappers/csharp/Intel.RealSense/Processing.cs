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
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame Colorize(VideoFrame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class Syncer : ProcessingBlock
    {
        public Syncer()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_sync_processing_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public void SubmitFrame(Frame f, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000, FramesReleaser releaser = null)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(queue.m_instance.Handle, timeout_ms, out error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(ptr));
        }

        public bool PollForFrames(out FrameSet result, FramesReleaser releaser = null)
        {
            object error;
            Frame f;
            if (NativeMethods.rs2_poll_for_frame(queue.m_instance.Handle, out f, out error) > 0)
            {
                result = FramesReleaser.ScopedReturn(releaser, new FrameSet(f.m_instance.Handle));
                f.Dispose();
                return true;
            }
            result = null;
            return false;
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class Align : ProcessingBlock
    {
        public Align(Stream align_to)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_align(align_to, out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public FrameSet Process(FrameSet original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrames() as FrameSet);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class DisparityTransform : ProcessingBlock
    {
        public DisparityTransform(bool transform_to_disparity = true)
        {
            object error;
            byte transform_direction = transform_to_disparity ? (byte)1 : (byte)0;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_disparity_transform_block(transform_direction, out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class DecimationFilter : ProcessingBlock
    {
        public DecimationFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_decimation_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class SpatialFilter : ProcessingBlock
    {
        public SpatialFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_spatial_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class TemporalFilter : ProcessingBlock
    {
        public TemporalFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_temporal_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class HoleFillingFilter : ProcessingBlock
    {
        public HoleFillingFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_hole_filling_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }

        readonly FrameQueue queue = new FrameQueue(1);
    }

    public class PointCloud : ProcessingBlock
    {
        readonly FrameQueue queue = new FrameQueue(1);

        public PointCloud()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public Points Calculate(Frame original, FramesReleaser releaser = null)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as Points);
        }

        public void MapTexture(VideoFrame texture)
        {
            object error;
            Options[Option.StreamFilter].Value = Convert.ToSingle(texture.Profile.Stream);
            Options[Option.StreamFormatFilter].Value = Convert.ToSingle(texture.Profile.Format);
            Options[Option.StreamIndexFilter].Value = Convert.ToSingle(texture.Profile.Index);
            NativeMethods.rs2_frame_add_ref(texture.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, texture.m_instance.Handle, out error);
        }
    }

    public class FrameSource
    {
        internal HandleRef m_instance;

        internal FrameSource(HandleRef instance)
        {
            m_instance = instance;
        }

        public VideoFrame AllocateVideoFrame(StreamProfile profile, Frame original, int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame )
        {
            object error;
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(m_instance.Handle, profile.m_instance.Handle, original.m_instance.Handle, bpp, width, height, stride, extension, out error);
            return new VideoFrame(fref);
        }

        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
        {
            object error;
            var frame_refs = frames.Select(x => x.m_instance.Handle).ToArray();
            foreach (var fref in frame_refs) NativeMethods.rs2_frame_add_ref(fref, out error);
            var frame_ref = NativeMethods.rs2_allocate_composite_frame(m_instance.Handle, frame_refs, frames.Count(), out error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(frame_ref));
        }

        public FrameSet AllocateCompositeFrame(params Frame[] frames)
        {
            return AllocateCompositeFrame(null, frames);
        }

        public void FrameReady(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_synthetic_frame_ready(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public void FramesReady(FrameSet fs)
        {
            using (var f = fs.AsFrame())
                FrameReady(f);
        }
    }

    public class CustomProcessingBlock : ProcessingBlock
    {
        public delegate void FrameProcessorCallback(Frame frame, FrameSource source);

        public CustomProcessingBlock(FrameProcessorCallback cb)
        {
            object error;
            frame_processor_callback cb2 = (IntPtr f, IntPtr src, IntPtr u) =>
            {
                using (var frame = new Frame(f))
                    cb(frame, new FrameSource(new HandleRef(this, src)));
            };
            m_proc_callback = cb2;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_processing_block_fptr(cb2, IntPtr.Zero, out error));
        }

        public void ProcessFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public void ProcessFrames(FrameSet fs)
        {
            using (var f = fs.AsFrame())
                ProcessFrame(f);
        }

        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);

            m_callback = null;
            m_queue = queue;
        }

        //public delegate void FrameCallback<Frame, T>(Frame frame, T user_data);
        public delegate void FrameCallback(Frame frame);

        public void Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = new Frame(f))
                    cb(frame);
            };
            NativeMethods.rs2_start_processing_fptr(m_instance.Handle, cb2, IntPtr.Zero, out error);
            m_callback = cb2;
            m_queue = null;
        }

        private frame_callback m_callback = null;
        private frame_processor_callback m_proc_callback = null;
        private FrameQueue m_queue = null;
    }
}