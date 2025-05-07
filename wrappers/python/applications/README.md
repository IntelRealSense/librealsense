# Intel RealSense - Object Camera

Objects are everywhere!

This package supports object data extraction from the intel Realsense camera RGB or Depth streams in real time.

The list of the supported objects, frameworks, platforms and applications are below. Click on each image to find out more.

# Objects

-  barcodes, QR Codes, Aruco Markers - well defined objects detected by using RGB camera data. 

barcodes   | QR Codes | Aruco Markers |
:------------: |  :----------: | :-------------:  |
[![barcode](barcode/doc/barcode_camera-ezgif.com-video-to-gif-converter.gif)](./barcode/README.md)  | [![QR Codes](./barcode/doc/qrcode_camera-ezgif.com-video-to-gif-converter.gif)](./barcode/README.md)  | [![Aruco](barcode/doc/aruco_camera-ezgif.com-video-to-gif-converter.gif)](./barcode/README.md)  |

-  Planes, Edges, Corners - 3D depth objects detected by using Depth data. 

Planes | Edges | Corners |
:------------: |  :----------: | :-------------:  |
[![Planes](Planes/doc/ezgif.com-animated-gif-maker.gif)](https://github.com/WorkIntel/Projects/blob/main/Planes/README.md)  | [![Edges](https://user-images.githubusercontent.com/32394882/230630901-9d53502a-f3f9-45b6-bf57-027148bb18ad.gif)](https://github.com/WorkIntel/Projects/blob/main/Planes/README.md)  | [![Corners](https://github.com/WorkIntel/Projects/blob/main/Planes/doc/Corner-ezgif.com-video-to-gif-converter.gif)](https://github.com/WorkIntel/Projects/blob/main/Planes/README.md)  |

-  General Object Detection - well defined objects detected by using Depth data. 

Object Motion Detection | Object Detection in 2D | 2D Objects using YOLO |
:------------: |  :----------: | :-------------:  |
[![Safety](Safety/doc/motion_detection-ezgif.com-video-to-gif-converter.gif)](Safety/README.md)  | [![Object Detection](https://user-images.githubusercontent.com/32394882/230630901-9d53502a-f3f9-45b6-bf57-027148bb18ad.gif)](https://www.stereolabs.com/docs/object-detection)  | [![Yolo](Yolo/doc/object_counting_output-ezgif.com-video-to-gif-converter.gif)](Yolo/README.md)  |

-  3D Pose estimation from RGB and Depth data. 

3D Pose Cup | 3D Pose Box | 3D Pose Metal Parts |
:------------: |  :----------: | :-------------:  |
[![Pose6D](Pose6D/doc/pose6d-ezgif.com-video-to-gif-converter.gif)](Pose6D/README.md)  | [![Pose6D-M](Pose6D/doc/RecordingMatch2024-08-08102001-ezgif.com-video-to-gif-converter.gif)](Pose6D/README.md)  | [![Pose6D-P](Pose6D/doc/object_0007_out-ezgif.com-video-to-gif-converter.gif)](Pose6D/README.md)  |


# Applications

User level applications supported by the Camera software

Region Detection | Object Counting | Motion Detection |
:------------: |  :----------: | :-------------:  |
[![Positional Tracking](https://user-images.githubusercontent.com/32394882/229093429-a445e8ae-7109-4995-bc1d-6a27a61bdb60.gif)](https://www.stereolabs.com/docs/positional-tracking/) | [![Global Localization](https://user-images.githubusercontent.com/32394882/230602944-ed61e6dd-e485-4911-8a4c-d6c9e4fab0fd.gif)](/global%20localization) | [![Motion](Safety/doc/motion_detection-ezgif.com-video-to-gif-converter.gif)](Safety/README.md) |

VSLAM/Localization | Plane Detection | Body Detection |
:------------: |  :----------: | :-------------:  |
[![Camera Control](https://user-images.githubusercontent.com/32394882/230602616-6b57c351-09c4-4aba-bdec-842afcc3b2ea.gif)](https://www.stereolabs.com/docs/video/camera-controls/) | [![Floor Detection](Planes/doc/ezgif.com-animated-gif-maker.gif)](Planes/README.md)  | [![Multi Camera Fusion](https://user-images.githubusercontent.com/32394882/228791106-a5f971d8-8d6f-483b-9f87-7f0f0025b8be.gif)](/fusion) |

# Request Camera Feature
If you want to run the application or object detection on the camera hardware - check this [link](https://docs.google.com/forms/d/e/1FAIpQLSdduDbnrRExDGFQqWAn8pX7jSr8KnwBmwuFOR9dgUabEp0F1A/viewform).

# Supported Platforms and Compute Environments

The following is the check list of supported environments and functionality:
- Windows
- Ubuntu
- Jetson (NVIDIA)
- Raspeberry PI
- RealSense AI Engine

# How to Contribute

We greatly appreciate contributions from the community, including examples, applications, and guides. If you'd like to contribute, please follow these guidelines:

1. **Create a pull request (PR)** with the title prefix `[RS]`, adding your new example folder to the `root/` directory within the repository.

2. **Ensure your project adheres to the following standards:**
   - Makes use of the `vision` package.
   - Includes a `README.md` with clear instructions for setting up and running the example.
   - Avoids adding large files or dependencies unless they are absolutely necessary for the example.




