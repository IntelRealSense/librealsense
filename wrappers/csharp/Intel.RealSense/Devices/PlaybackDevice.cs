using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class PlaybackDevice : Device
    {
        internal PlaybackDevice(IntPtr dev) : base(dev)
        {

        }


        protected override void Dispose(bool disposing)
        {
            // Intentionally empty, does not own the native device, only wraps it.
        }

        public static PlaybackDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.m_instance, Extension.Playback, out error) == 0)
            {
                throw new ArgumentException("Device does not support Playback");
            }

            return new PlaybackDevice(dev.m_instance);
        }

        public void Pause()
        {
            object error;
            NativeMethods.rs2_playback_device_pause(m_instance, out error);
        }

        public void Resume()
        {
            object error;
            NativeMethods.rs2_playback_device_resume(m_instance, out error);
        }

        public PlaybackStatus Status
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_get_current_status(m_instance, out error);
            }
        }

        public ulong Duration
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_duration(m_instance, out error);
            }
        }

        public ulong Position
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_position(m_instance, out error);
            }
            set
            {
                object error;
                NativeMethods.rs2_playback_seek(m_instance, (long)value, out error);
            }
        }

        public void Seek(long time)
        {
            object error;
            NativeMethods.rs2_playback_seek(m_instance, time, out error);
        }

        public bool Realtime
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_is_real_time(m_instance, out error) != 0;
            }
            set
            {
                object error;
                NativeMethods.rs2_playback_device_set_real_time(m_instance, value ? 1 : 0, out error);
            }
        }

        public float Speed
        {
            set
            {
                object error;
                NativeMethods.rs2_playback_device_set_playback_speed(m_instance, value, out error);
            }
        }
    }
}
