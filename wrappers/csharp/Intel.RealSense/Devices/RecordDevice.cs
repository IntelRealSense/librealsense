// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    public class RecordDevice : Device
    {
        internal RecordDevice(IntPtr ptr)
            : base(ptr)
        {
        }

        internal static IntPtr Create(Device dev, string file)
        {
            object error;
            return NativeMethods.rs2_create_record_device(dev.Handle, file, out error);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RecordDevice"/> class.
        /// </summary>
        /// <param name="dev">The <see cref="Device"/> to record</param>
        /// <param name="file">The desired path to which the recorder should save the data</param>
        public RecordDevice(Device dev, string file)
            : base(Create(dev, file), NativeMethods.rs2_delete_device)
        {
        }

        /// <summary>
        ///  Create a <see cref="RecordDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.Record"/></param>
        /// <returns>a new <see cref="RecordDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.Record"/></exception>
        public static RecordDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.Record, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.Record)}");
            }

            return Device.Create<RecordDevice>(dev.Handle);
        }

        /// <summary>
        /// Pause the recording device without stopping the actual device from streaming.
        /// Pausing will cause the device to stop writing new data to the file, in particular, frames and changes to extensions
        /// </summary>
        public void Pause()
        {
            object error;
            NativeMethods.rs2_record_device_pause(Handle, out error);
        }

        /// <summary>
        /// Unpause the recording device. Resume will cause the device to continue writing new data to the file, in particular, frames and changes to extensions
        /// </summary>
        public void Resume()
        {
            object error;
            NativeMethods.rs2_record_device_resume(Handle, out error);
        }

        /// <summary>
        /// Gets the name of the file to which the recorder is writing
        /// </summary>
        public string FileName
        {
            get
            {
                object error;
                var p = NativeMethods.rs2_record_device_filename(Handle, out error);
                return Marshal.PtrToStringAnsi(p);
            }
        }
    }
}
