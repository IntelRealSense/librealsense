using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Devices
{
    public class RecordDevice : Device
    {
        public string Filename
        {
            get
            {
                var p = NativeMethods.rs2_record_device_filename(Instance, out var error);
                return Marshal.PtrToStringAnsi(p);
            }
        }

        private readonly IntPtr dev;

        public RecordDevice(Device dev, string file) : base(IntPtr.Zero)
        {
            this.dev = dev.Instance;
            Instance = NativeMethods.rs2_create_record_device(this.dev, file, out var error);
        }

        public void Pause()
            => NativeMethods.rs2_record_device_pause(Instance, out var error);

        public void Resume()
            => NativeMethods.rs2_record_device_resume(Instance, out var error);
    }
}
