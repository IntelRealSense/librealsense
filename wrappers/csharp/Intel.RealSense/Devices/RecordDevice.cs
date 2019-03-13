using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class RecordDevice : Device
    {
        internal RecordDevice(IntPtr ptr) : base(ptr)
        {
        }

        internal static IntPtr Create(Device dev, string file)
        {
            object error;
            return NativeMethods.rs2_create_record_device(dev.Handle, file, out error);
        }

        public RecordDevice(Device dev, string file) : base(Create(dev, file))
        {
        }

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
        public string Filename
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
