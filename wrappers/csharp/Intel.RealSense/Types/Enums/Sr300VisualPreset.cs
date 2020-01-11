// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// For SR300 devices: provides optimized settings (presets) for specific types of usage.
    /// </summary>
    public enum Sr300VisualPreset
    {
        /// <summary> Preset for short range</summary>
        ShortRange = 0,

        /// <summary> Preset for long range</summary>
        LongRange = 1,

        /// <summary> Preset for background segmentation</summary>
        BackgroundSegmentation = 2,

        /// <summary> Preset for gesture recognition</summary>
        GestureRecognition = 3,

        /// <summary> Preset for object scanning</summary>
        ObjectScanning = 4,

        /// <summary> Preset for face analytics</summary>
        FaceAnalytics = 5,

        /// <summary> Preset for face login</summary>
        FaceLogin = 6,

        /// <summary> Preset for GR cursor</summary>
        GrCursor = 7,

        /// <summary> Camera default settings</summary>
        Default = 8,

        /// <summary> Preset for mid-range</summary>
        MidRange = 9,

        /// <summary> Preset for IR only</summary>
        IrOnly = 10,
    }
}
