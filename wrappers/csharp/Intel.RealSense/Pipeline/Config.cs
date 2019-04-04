// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The config allows pipeline users to request filters for the pipeline streams and device selection and configuration.
    /// </summary>
    public class Config : Base.Object
    {
        internal static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_config(out error);
        }

        public Config()
            : base(Create(), NativeMethods.rs2_delete_config)
        {
        }

        public void EnableStream(Stream s, int index = -1)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(Handle, s, index, 0, 0, Format.Any, 0, out error);
        }

        public void EnableStream(Stream stream_type, int stream_index, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(Handle, stream_type, stream_index, width, height, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, int width, int height, Format format = Format.Any, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(Handle, stream_type, -1, width, height, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, Format format, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(Handle, stream_type, -1, 0, 0, format, framerate, out error);
        }

        public void EnableStream(Stream stream_type, int stream_index, Format format, int framerate = 0)
        {
            object error;
            NativeMethods.rs2_config_enable_stream(Handle, stream_type, stream_index, 0, 0, format, framerate, out error);
        }

        public void EnableAllStreams()
        {
            object error;
            NativeMethods.rs2_config_enable_all_stream(Handle, out error);
        }

        public void EnableDevice(string serial)
        {
            object error;
            NativeMethods.rs2_config_enable_device(Handle, serial, out error);
        }

        public void EnableDeviceFromFile(string filename)
        {
            object error;
            NativeMethods.rs2_config_enable_device_from_file(Handle, filename, out error);
        }

        public void EnableDeviceFromFile(string filename, bool repeat)
        {
            object error;
            NativeMethods.rs2_config_enable_device_from_file_repeat_option(Handle, filename, repeat ? 1 : 0, out error);
        }

        public void EnableRecordToFile(string filename)
        {
            object error;
            NativeMethods.rs2_config_enable_record_to_file(Handle, filename, out error);
        }

        public void DisableStream(Stream s, int index = -1)
        {
            object error;
            NativeMethods.rs2_config_disable_indexed_stream(Handle, s, index, out error);
        }

        public void DisableAllStreams()
        {
            object error;
            NativeMethods.rs2_config_disable_all_streams(Handle, out error);
        }

        public bool CanResolve(Pipeline pipe)
        {
            object error;
            var res = NativeMethods.rs2_config_can_resolve(Handle, pipe.Handle, out error);
            return res > 0;
        }

        public PipelineProfile Resolve(Pipeline pipe)
        {
            object error;
            var res = NativeMethods.rs2_config_resolve(Handle, pipe.Handle, out error);
            return new PipelineProfile(res);
        }
    }
}
