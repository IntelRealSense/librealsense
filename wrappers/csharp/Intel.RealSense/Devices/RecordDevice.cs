using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class RecordDevice : Device
    {
        readonly IntPtr m_dev;

        public RecordDevice(Device dev, string file) : base(IntPtr.Zero)
        {
            m_dev = dev.m_instance;
            object error;
            m_instance = NativeMethods.rs2_create_record_device(m_dev, file, out error);
        }

        public void Pause()
        {
            object error;
            NativeMethods.rs2_record_device_pause(m_instance, out error);
        }

        public void Resume()
        {
            object error;
            NativeMethods.rs2_record_device_resume(m_instance, out error);
        }

        public string Filename
        {
            get
            {
                object error;
                var p = NativeMethods.rs2_record_device_filename(m_instance, out error);
                return Marshal.PtrToStringAnsi(p);
            }
        }
    }
}
