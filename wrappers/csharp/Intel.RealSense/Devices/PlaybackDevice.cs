using Intel.RealSense.Types;
using System;

namespace Intel.RealSense.Devices
{
    public class PlaybackDevice : Device
    {
        public PlaybackStatus Status => NativeMethods.rs2_playback_device_get_current_status(Instance, out var error);
        public ulong Duration => NativeMethods.rs2_playback_get_duration(Instance, out var error);
        public ulong Position
        {
            get => NativeMethods.rs2_playback_get_position(Instance, out var error);
            set => NativeMethods.rs2_playback_seek(Instance, (long)value, error: out var error);
        }
        public bool Realtime
        {
            get => NativeMethods.rs2_playback_device_is_real_time(Instance, out var error) != 0;
            set => NativeMethods.rs2_playback_device_set_real_time(Instance, value ? 1 : 0, error: out var error);
        }
        public float Speed
        {
            set => NativeMethods.rs2_playback_device_set_playback_speed(Instance, value, out var error);
        }

        internal PlaybackDevice(IntPtr dev) : base(dev)
        {

        }

        public void Pause()
            => NativeMethods.rs2_playback_device_pause(Instance, out var error);

        public void Resume()
            => NativeMethods.rs2_playback_device_resume(Instance, out var error);

        public void Seek(long time)
            => NativeMethods.rs2_playback_seek(Instance, time, out var error);

        public static PlaybackDevice FromDevice(Device dev)
        {
            if (NativeMethods.rs2_is_device_extendable_to(dev.Instance, Extension.Playback, out var error) == 0)
            {
                throw new ArgumentException("Device does not support Playback");
            }

            return new PlaybackDevice(dev.Instance);
        }

        protected override void Dispose(bool disposing)
        {
            // Intentionally empty, does not own the native device, only wraps it.
        }
    }
}
