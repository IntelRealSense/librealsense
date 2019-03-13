using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Pipeline : Base.Object
    {
        private frame_callback m_callback;

        internal static IntPtr Create(Context ctx)
        {
            object error;
            return NativeMethods.rs2_create_pipeline(ctx.Handle, out error);
        }

        public Pipeline(Context ctx) : base(Create(ctx), NativeMethods.rs2_delete_pipeline)
        {
        }

        public Pipeline() : base(Create(new Context()), NativeMethods.rs2_delete_pipeline)
        {
        }

        /// <summary>Start the pipeline streaming with its default configuration.</summary>
        /// <remarks>
        /// Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.
        /// </remarks>
        /// <returns>The actual pipeline device and streams profile, which was successfully configured to the streaming device.</returns>
        public PipelineProfile Start()
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start(Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(Config cfg)
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start_with_config(Handle, cfg.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                    cb(frame);
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_callback(Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(Config cfg, FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                    cb(frame);
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_config_and_callback(Handle, cfg.Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public void Stop()
        {
            object error;
            NativeMethods.rs2_pipeline_stop(Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000u)
        {
            object error;
            var ptr = NativeMethods.rs2_pipeline_wait_for_frames(Handle, timeout_ms, out error);
            return FrameSet.Create(ptr);
        }

        public bool TryWaitForFrames(out FrameSet frames, uint timeout_ms = 5000u)
        {
            object error;
            IntPtr ptr;
            bool res = NativeMethods.rs2_pipeline_try_wait_for_frames(Handle, out ptr, timeout_ms, out error) > 0;
            frames = res ? FrameSet.Create(ptr) : null;
            return res;
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr fs;
            if (NativeMethods.rs2_pipeline_poll_for_frames(Handle, out fs, out error) > 0)
            {
                result = FrameSet.Create(fs);
                return true;
            }
            result = null;
            return false;
        }

        /// <summary>Return the active device and streams profiles, used by the pipeline.</summary>
        /// <remarks>The method returns a valid result only when the pipeline is active</remarks>
        /// <value>The actual pipeline device and streams profile, which was successfully configured to the streaming device on start.</value>
        public PipelineProfile ActiveProfile
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_get_active_profile(Handle, out error);
                return new PipelineProfile(ptr);
            }
        }
    }
}
