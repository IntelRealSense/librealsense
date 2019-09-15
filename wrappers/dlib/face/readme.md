# rs-face-dlib Sample

## Overview
This example demonstrates a very simple facial landmark detection using
[Dlib](http://dlib.net/)'s machine learning algorithms, using depth data to
implement basic anti-spoofing.

> **Note:** This is just an example intended to showcase possible applications.
> The heuristics used here are very simple and basic, and can be greatly
> improved on.

<p align="center"><img src="rs-face-dlib.jpg" alt="screenshot"/></p>

Faces detected by the camera will show landmarks in either green or red:

* Green landmarks indicate a "real" face with depth data corroborating
  expectations. E.g., the distance to the eyes should be greater than to the tip
  of the nose, etc.

* Red landmarks indicate a "fake" face. E.g., a *picture* of a face, where the
  distance to each facial feature does not meet expectations

> Note: faces should be forward-facing to be detectable

## Implementation

To enable usage of librealsense frame data as a dlib image, a `rs_frame_image`
class is introduced. No copying of frame data takes place.

```cpp
rs_frame_image< dlib::rgb_pixel, RS2_FORMAT_RGB8 > image( color_frame );
```

Faces are detected in two steps:

1. Facial boundary rectangles are detected:
```cpp
  dlib::frontal_face_detector face_bbox_detector = dlib::get_frontal_face_detector();
  ...
  std::vector< dlib::rectangle > face_bboxes = face_bbox_detector( image );
```

2. Each one is then annotated to find its landmarks:
```cpp
  dlib::shape_predictor face_landmark_annotator;
  dlib::deserialize( "shape_predictor_68_face_landmarks.dat" ) >> face_landmark_annotator;
  ...
  std::vector< dlib::full_object_detection > faces;
  for( auto const & bbox : face_bboxes )
    faces.push_back( face_landmark_annotator( image, bbox ));
```

> **Note:** A dataset (a trained model file) is required to run this sample.
> You can use a dataset of your choosing/choosing or download
the [68-point trained model file from dlib's site](http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2).

The landmarks calculated are 68-point 2D coordinates in the color frame. See
[this picture](https://ibug.doc.ic.ac.uk/media/uploads/images/annotpics/figure_68_markup.jpg)
for (1-based) indexes for each of the points.

Once available, landmarks are used to get average eye, nose, ear, mouth, and
chin depths. The distances are compared with expected relationships:

```cpp
if( nose_depth >= eye_depth )
    return false;
if( eye_depth - nose_depth > 0.07f )
    return false;
if( ear_depth <= eye_depth )
    return false;
if( mouth_depth <= nose_depth )
    return false;
if( mouth_depth > chin_depth )
    return false;

// All the distances, collectively, should not span a range that makes no sense. I.e.,
// if the face accounts for more than 20cm of depth, or less than 2cm, then something's
// not kosher!
float x = std::max( { nose_depth, eye_depth, ear_depth, mouth_depth, chin_depth } );
float n = std::min( { nose_depth, eye_depth, ear_depth, mouth_depth, chin_depth } );
if( x - n > 0.20f )
    return false;
if( x - n < 0.02f )
    return false;
```

Because depth data is used to correspond with landmark pixels from the
color image, it is important that the depth and color frames from the camera
are aligned:

```cpp
rs2::align align_to_color( RS2_STREAM_COLOR );
...
rs2::frameset data = pipe.wait_for_frames();
data = align_to_color.process( data );       // Replace with aligned frames
```

The depth data is accessed directly, for speed, instead of relying
`depth_frame.get_distance()` which may incur overhead.
