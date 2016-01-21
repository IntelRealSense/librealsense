# Developer Log 

### October 20th, 2015

1. (R200) ALL of 640x480, 628x468, 492x372, 480x360, 332x252, 320x240 are now available for DEPTH and INFRARED streams.
2. (All) It is safe to hardcode 640x480 or 320x240 as DEPTH or INFRARED resolutions.
3. (R200) For the resolutions which are 12 pixels larger than the native depth resolution, the depth image is centered and padded.
4. (R200) For the resolutions which are 12 pixels smaller than the native infrared resolution, the IR image is cropped and centered.
5. (All) If the same resolution is requested for DEPTH and INFRARED streams, they will be pixel-for-pixel aligned and have the same intrinsics.
6. New stream mode: RECTIFIED_COLOR – Equivalent to DSAPI’s rectified third modes, removes COLOR image distortion and cancels out rotation relative to DEPTH stream.
  * Suitable for use as a background for augmented reality rendering, images produced via rectilinear perspective projection will overlay RECTIFIED_COLOR images correctly.
7. New stream mode: DEPTH_ALIGNED_TO_RECTIFIED_COLOR – Maps data coming from the DEPTH stream to match the intrinsic and extrinsic camera properties of the DEPTH stream.
  * Suitable for pre-loading the depth map for augmented reality rendering, or performing tracking that needs to be pixel accurate with the RECTIFIED_COLOR image.