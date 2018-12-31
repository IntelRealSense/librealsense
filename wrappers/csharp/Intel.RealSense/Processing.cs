using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public interface IProcessingBlock : IOptions
    {
        Frame Process(Frame original);
        FrameSet Process(FrameSet original);
    }

    public abstract class ProcessingBlock : IProcessingBlock, IDisposable
    {
        public readonly FrameQueue queue = new FrameQueue(1);

        internal HandleRef m_instance;

        Sensor.SensorOptions m_options;
        public IOptionsContainer Options
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

        /// <summary>
        /// Process frame and return the result
        /// </summary>
        /// <param name="original"></param>
        /// <returns></returns>
        public Frame Process(Frame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            Frame f;
            if (queue.PollForFrame(out f))
            {
                return f;
            }
            return original;
        }

        public FrameSet Process(FrameSet original)
        {
            FrameSet rv;
            using (var singleOriginal = original.AsFrame())
            {
                using (var processed = Process(singleOriginal))
                {
                    rv = FrameSet.FromFrame(processed);
                }
            }
            return rv;
        }
    }

    public static class IProcessingBlockExtensions
    {
        public static Frame ApplyFilter(this Frame frame, IProcessingBlock block)
        {
            return block.Process(frame);
        }

        public static FrameSet ApplyFilter(this FrameSet frames, IProcessingBlock block)
        {
            return block.Process(frames);
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

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame Colorize(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class Syncer : ProcessingBlock
    {
        public Syncer()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_sync_processing_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public void SubmitFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(queue.m_instance.Handle, timeout_ms, out error);
            return FrameSet.Pool.Get(ptr);
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(queue.m_instance.Handle, out ptr, out error) > 0)
            {
                result = FrameSet.Pool.Get(ptr);
                return true;
            }
            result = null;
            return false;
        }
    }

    public class Align : ProcessingBlock
    {
        public Align(Stream align_to)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_align(align_to, out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process with DisposeWith method instead")]
        public FrameSet Process(FrameSet original, FramesReleaser releaser)
        {
            return Process(original).DisposeWith(releaser);
        }
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

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class DecimationFilter : ProcessingBlock
    {
        public DecimationFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_decimation_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class SpatialFilter : ProcessingBlock
    {
        public SpatialFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_spatial_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class TemporalFilter : ProcessingBlock
    {
        public TemporalFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_temporal_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class HoleFillingFilter : ProcessingBlock
    {
        public HoleFillingFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_hole_filling_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }

    public class PointCloud : ProcessingBlock
    {
        private readonly IOption formatFilter;
        private readonly IOption indexFilter;
        private readonly IOption streamFilter;

        public PointCloud()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);

            streamFilter = Options[Option.StreamFilter];
            formatFilter = Options[Option.StreamFormatFilter];
            indexFilter = Options[Option.StreamIndexFilter];
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public Points Calculate(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as Points;
        }

        public void MapTexture(VideoFrame texture)
        {
            using (var p = texture.Profile) {
                streamFilter.Value = (float)p.Stream;
                formatFilter.Value = (float)p.Format;
                indexFilter.Value = (float)p.Index;
            }
            using (var f = Process(texture));
        }
    }

    public struct FrameSource
    {
        internal readonly HandleRef m_instance;

        internal FrameSource(HandleRef instance)
        {
            m_instance = instance;
        }

        public T AllocateVideoFrame<T>(StreamProfile profile, Frame original, 
            int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame) where T : Frame
        {
            object error;
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(m_instance.Handle, profile.m_instance.Handle, original.m_instance.Handle, bpp, width, height, stride, extension, out error);
            return Frame.CreateFrame(fref) as T;
        }

        [Obsolete("This method is obsolete. Use AllocateCompositeFrame with DisposeWith method instead")]
        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames).DisposeWith(releaser);
        }

        public FrameSet AllocateCompositeFrame(params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames);
        }


        public FrameSet AllocateCompositeFrame(IList<Frame> frames)
        {
            if (frames == null)
                throw new ArgumentNullException(nameof(frames));

            IntPtr frame_refs = IntPtr.Zero;

            try {
                object error;
                int fl = frames.Count;
                frame_refs = Marshal.AllocHGlobal(fl * IntPtr.Size);
                for (int i = 0; i < fl; i++)
                {
                    var fr = frames[i].m_instance.Handle;
                    Marshal.WriteIntPtr(frame_refs, i * IntPtr.Size, fr);
                    NativeMethods.rs2_frame_add_ref(fr, out error);
                }

                var frame_ref = NativeMethods.rs2_allocate_composite_frame(m_instance.Handle, frame_refs, fl, out error);
                return FrameSet.Pool.Get(frame_ref);
            }
            finally
            {
                if (frame_refs != IntPtr.Zero)
                    Marshal.FreeHGlobal(frame_refs);
            }
        }

        private void FrameReady(IntPtr ptr)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(ptr, out error);
            NativeMethods.rs2_synthetic_frame_ready(m_instance.Handle, ptr, out error);
        }

        public void FrameReady(Frame f)
        {
            FrameReady(f.m_instance.Handle);
        }

        public void FramesReady(FrameSet fs)
        {
            using (fs)
                FrameReady(fs.m_instance.Handle);
        }
    }

    public class CustomProcessingBlock : ProcessingBlock
    {
        public delegate void FrameProcessorCallback(Frame frame, FrameSource source);

        public CustomProcessingBlock(FrameProcessorCallback cb)
        {
            frameProcessorCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameProcessorCallbackHandle);

            object error;
            var pb = NativeMethods.rs2_create_processing_block_fptr(fpc, cbPtr, out error);
            m_instance = new HandleRef(this, pb);
        }

        readonly frame_processor_callback fpc = new frame_processor_callback(ProcessingBlockCallback);

        static void ProcessingBlockCallback(IntPtr f, IntPtr src, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameProcessorCallback;
                using (var frame = Frame.CreateFrame(f))
                    callback(frame, new FrameSource(new HandleRef(frame, src)));
            }
            // ArgumentException: GCHandle value belongs to a different domain
            // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            catch (ArgumentException) { }
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

        /// <summary>
        /// Start the processing block, delivering frames to external queue
        /// </summary>
        /// <param name="queue"></param>
        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        /// <summary>
        /// Start the processing block
        /// </summary>
        public void Start()
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public delegate void FrameCallback(Frame frame);

        /// <summary>
        /// Start the processing block, delivering frames to a callback
        /// </summary>
        /// <param name="cb"></param>
        public void Start(FrameCallback cb)
        {
            frameCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameCallbackHandle);

            object error;
            NativeMethods.rs2_start_processing_fptr(m_instance.Handle, m_frameCallback, cbPtr, out error);
        }

        readonly frame_callback m_frameCallback = new frame_callback(ProcessingBlockFrameCallback);
        static void ProcessingBlockFrameCallback(IntPtr f, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameCallback;
                using (var frame = Frame.CreateFrame(f))
                    callback(frame);
            }
            // ArgumentException: GCHandle value belongs to a different domain
            // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            catch (ArgumentException) { }
        }

        private GCHandle frameCallbackHandle;
        private readonly GCHandle frameProcessorCallbackHandle;

        protected override void Dispose(bool disposing)
        {
            //if (!disposedValue)
            {
                if (disposing)
                {
                    if (frameCallbackHandle.IsAllocated)
                        frameCallbackHandle.Free();

                    if (frameProcessorCallbackHandle.IsAllocated)
                        frameProcessorCallbackHandle.Free();
                }
            }

            base.Dispose(disposing);
        }
    }
}