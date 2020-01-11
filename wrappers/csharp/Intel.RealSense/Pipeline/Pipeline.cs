// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The pipeline simplifies the user interaction with the device and computer vision processing modules.
    /// </summary>
    /// <remarks>
    /// The class abstracts the camera configuration and streaming, and the vision modules triggering and threading.
    /// It lets the application focus on the computer vision output of the modules, or the device output data.
    /// The pipeline can manage computer vision modules, which are implemented as a processing blocks.
    /// The pipeline is the consumer of the processing block interface, while the application consumes the
    /// computer vision interface.
    /// </remarks>
    public class Pipeline : Base.Object
    {
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private frame_callback m_callback;

        internal static IntPtr Create(Context ctx)
        {
            object error;
            return NativeMethods.rs2_create_pipeline(ctx.Handle, out error);
        }

        internal static IntPtr Create()
        {
            using (var ctx = new Context())
            {
                return Create(ctx);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Pipeline"/> class.
        /// </summary>
        /// <param name="ctx">context</param>
        public Pipeline(Context ctx)
            : base(Create(ctx), NativeMethods.rs2_delete_pipeline)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Pipeline"/> class.
        /// </summary>
        public Pipeline()
            : base(Create(), NativeMethods.rs2_delete_pipeline)
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

        /// <summary>
        /// Start the pipeline streaming according to the configuraion.
        /// </summary>
        /// <param name="cfg">A <see cref="Config"/> with requested filters on the pipeline configuration. By default no filters are applied.</param>
        /// <returns>The actual pipeline device and streams profile, which was successfully configured to the streaming device.</returns>
        public PipelineProfile Start(Config cfg)
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start_with_config(Handle, cfg.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        /// <summary>
        /// Start the pipeline streaming with its default configuration.
        /// <para>
        /// The pipeline captures samples from the device, and delivers them to the through the provided frame callback.
        /// </para>
        /// </summary>
        /// <remarks>
        /// Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.
        /// When starting the pipeline with a callback both <see cref="WaitForFrames"/> or <see cref="PollForFrames"/> will throw exception.
        /// </remarks>
        /// <param name="cb">Delegate to register as per-frame callback</param>
        /// <returns>The actual pipeline device and streams profile, which was successfully configured to the streaming device.</returns>
        // TODO: overload with state object and Action<Frame, object> callback to avoid allocations
        public PipelineProfile Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                {
                    cb(frame);
                }
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_callback(Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        // TODO: overload with state object and Action<Frame, object> callback to avoid allocations
        public PipelineProfile Start(Config cfg, FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                {
                    cb(frame);
                }
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_config_and_callback(Handle, cfg.Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        /// <summary>
        /// Stop the pipeline streaming.
        /// </summary>
        /// <remarks>
        /// The pipeline stops delivering samples to the attached computer vision modules and processing blocks, stops the device streaming
        /// and releases the device resources used by the pipeline. It is the application's responsibility to release any frame reference it owns.
        /// The method takes effect only after <see cref="Start"/> was called, otherwise an exception is raised.
        /// </remarks>
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

        /// <summary>Gets the active device and streams profiles, used by the pipeline.</summary>
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
