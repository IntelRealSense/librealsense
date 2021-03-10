// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    // TODO: rs2_create_playback_device
    // TODO: rs2_playback_device_set_status_changed_callback
    public class PlaybackDevice : Device
    {
        internal PlaybackDevice(IntPtr dev)
            : base(dev)
        {
        }

        /// <summary>
        /// Create a <see cref="PlaybackDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.Playback"/></param>
        /// <returns>a new <see cref="PlaybackDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.Playback"/></exception>
        public static PlaybackDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.Playback, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.Playback)}");
            }

            var playback = Device.Create<PlaybackDevice>(dev.Handle);
            playback.FileName = Marshal.PtrToStringAnsi(NativeMethods.rs2_playback_device_get_file_path(dev.Handle, out error));
            return playback;
        }

        /// <summary>
        /// Gets the path of the file used by the playback device
        /// </summary>
        /// <value>Path to the file used by the playback device</value>
        public string FileName { get; private set; }

        /// <summary>
        /// Pauses the playback
        /// </summary>
        /// <remarks>
        /// Calling Pause() in "Paused" status does nothing
        /// If Pause() is called while playback status is "Playing" or "Stopped", the playback will not play until Resume() is called
        /// </remarks>
        public void Pause()
        {
            object error;
            NativeMethods.rs2_playback_device_pause(Handle, out error);
        }

        /// <summary>
        /// Un-pauses the playback
        /// </summary>
        /// <remarks>
        /// Calling Resume() while playback status is "Playing" or "Stopped" does nothing
        /// </remarks>
        public void Resume()
        {
            object error;
            NativeMethods.rs2_playback_device_resume(Handle, out error);
        }

        /// <summary>
        /// Stops the playback
        /// </summary>
        /// <remarks>
        /// Calling Stop() will stop all streaming playback sensors and will reset the playback(returning to beginning of file)
        /// </remarks>
        public void Stop()
        {
            object error;
            NativeMethods.rs2_playback_device_stop(Handle, out error);
        }

        /// <summary>
        /// Gets the current state of the playback device
        /// </summary>
        /// <value>Current state of the playback</value>
        public PlaybackStatus Status
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_get_current_status(Handle, out error);
            }
        }

        /// <summary>
        /// Gets the total duration of the file in units of nanoseconds</summary>
        /// <value>
        /// Total duration of the file in units of nanoseconds
        /// </value>
        public ulong Duration
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_duration(Handle, out error);
            }
        }

        /// <summary>
        /// Gets or sets the current position of the playback in the file in terms of time. Units are expressed in nanoseconds
        /// </summary>
        /// <value>
        /// Current position of the playback in the file in terms of time. Units are expressed in nanoseconds
        /// </value>
        public ulong Position
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_position(Handle, out error);
            }

            set
            {
                object error;
                NativeMethods.rs2_playback_seek(Handle, (long)value, out error);
            }
        }

        /// <summary>
        /// Set the playback to a specified time point of the played data
        /// </summary>
        /// <param name="time">The time point to which playback should seek, expressed in units of nanoseconds (zero value = start)</param>
        public void Seek(long time)
        {
            object error;
            NativeMethods.rs2_playback_seek(Handle, time, out error);
        }

        /// <summary>
        /// Gets or sets a value indicating whether the playback works in real time or non real time
        /// </summary>
        /// <value>Indicates if playback is in real time mode or non real time</value>
        /// <remarks>
        /// In real time mode, playback will play the same way the file was recorded.
        /// In real time mode if the application takes too long to handle the callback, frames may be dropped.
        /// In non real time mode, playback will wait for each callback to finish handling the data before
        /// reading the next frame. In this mode no frames will be dropped, and the application controls the
        /// frame rate of the playback (according to the callback handler duration).
        /// </remarks>
        public bool Realtime
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_is_real_time(Handle, out error) != 0;
            }

            set
            {
                object error;
                NativeMethods.rs2_playback_device_set_real_time(Handle, value ? 1 : 0, out error);
            }
        }

        /// <summary>
        /// Set the playing speed
        /// </summary>
        /// <param name="speed">Indicates a multiplication of the speed to play (e.g: 1 = normal, 0.5 twice as slow)</param>
        public void SetSpeed(float speed)
        {
            object error;
            NativeMethods.rs2_playback_device_set_playback_speed(Handle, speed, out error);
        }
    }
}
