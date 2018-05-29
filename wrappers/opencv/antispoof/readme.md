# rs-antispoof Sample

## Overview

This tutorial shows how a relatively complex computer vision task can be solved with ease when depth is available

**The problem**: distinguish between a human face and a printed picture of a human face

> **Disclaimer**: Like with rest of SDK tutorials the goal is to provide an educational example that can help users get started, rather than a production-ready software.
> For that purpose the code is kept short and readable and many corner cases unhandled.
> A real **anti-spoofing** algorithm would probably use machine learning on depth data, instead of simple plane-fitting and RMS. However, the overall idea would remain the same. 

## Expected Output

<p align="center"><img src="https://github.com/dorodnic/librealsense/wiki/antispoof.gif" /><br/>opencv face detection with basic anti-spoofing based on depth data</p>

## How it works

We are using opencv [CascadeClassifier](http://opencvexamples.blogspot.com/2013/10/face-detection-using-haar-cascade.html) to detect all the faces in the RGB stream:

```cpp
CascadeClassifier face_cascade;
face_cascade.load("haarcascade_frontalface_alt2.xml");

std::vector<Rect> faces;
face_cascade.detectMultiScale(color_mat, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
```

Then we are using the SDK to try fit a plane to each face using the corresponding aligned depth frame:
```cpp
depth.fit_plane(roi, 2, 0.05f, a, b, c, d, rms)
```
If fitting was succesful, we check the quality of the fit via the `rms` output variable. This variable represents the spread of depth data around the plane in meters. 
Anything bellow 2cm we report as a 2D picture of a face.

This approach is invariant to picture tilt and rotation (since plane-fit will compensate for any rigid transformation), but can be easily beaten by bending the picture. 
