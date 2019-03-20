// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// State of a <see cref="PlaybackDevice"/>
    /// </summary>
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
