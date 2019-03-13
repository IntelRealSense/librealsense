using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum PlaybackStatus
    {
        /// <summary>Unknown state</summary>
        Unknown = 0,
        /// <summary>One or more sensors were started, playback is reading and raising data</summary>
        Playing = 1,
        /// <summary>One or more sensors were started, but playback paused reading and paused raising dat</summary>
        Paused = 2,
        /// <summary>All sensors were stopped, or playback has ended (all data was read). This is the initial playback status</summary>
        Stopped = 3,
    }
}
