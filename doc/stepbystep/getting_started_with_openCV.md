# Getting Started

* [Displaying Color Frame](#Displaying-Color-Frame)
* [Displaying Infrared Frame](#Displaying-Infrared-Frame)

## Displaying Color Frame

In this demo, you will acquire color frame from the RealSense camera and display it using OpenCV. 
Before you start, make sure you have librealsense and OpenCV installed and working properly on your system.
Using the editor of your choise create BGR_sample.cpp and copy-paste the following code-snippet: 

```cpp
// include the librealsense C++ header file
#include <librealsense/rs.hpp>

// include OpenCV header file
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main()
{
    // Create a context object. This object owns the handles to all connected realsense devices
    rs::context ctx;

    // Access the first available RealSense device
    rs::device * dev = ctx.get_device(0);

    // Configure Infrared stream to run at VGA resolution at 30 frames per second
    dev->enable_stream(rs::stream::color, 640, 480, rs::format::bgr8, 30);

    // Start streaming
    dev->start();

    // Camera warmup - Dropped several first frames to let auto-exposure stabilize
    for(int i = 0; i < 30; i++)
       dev->wait_for_frames();

    // Creating OpenCV Matrix from a color image
    Mat color(Size(640, 480), CV_8UC3, (void*)dev->get_frame_data(rs::stream::color), Mat::AUTO_STEP);

    // Display in a GUI
    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", color);

    waitKey(0);

    return 0;
}
```

Compile and run the application from terminal using the following command line: 

```shell
g++ -std=c++11 BGR_sample.cpp -lrealsense –lopencv_core -lopencv_highgui -o BGR && ./BGR
```

**Result:**

![BGR_Image](./resources/Image_BGR.png)


## Displaying Infrared Frame

Displaying Infrared and Depth data is not very different from Color data. Infrared frame is a single channel, 8 bits-per-pixel image. 
Copy the following code snippet into IR_sample.cpp:

```cpp
// include the librealsense C++ header file
#include <librealsense/rs.hpp>

// include OpenCV header file
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main()
{
    rs::context ctx;
    rs::device * dev = ctx.get_device(0);

    // Configure Infrared stream to run at VGA resolution at 30 frames per second
    dev->enable_stream(rs::stream::infrared, 640, 480, rs::format::y8, 30);

    // We must also configure depth stream in order to IR stream run properly
    dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);

    // Start streaming
    dev->start();

    // Camera warmup - Dropped frames to allow stabilization
    for(int i = 0; i < 40; i++)
        dev->wait_for_frames();

    // Creating OpenCV matrix from IR image
    Mat ir(Size(640, 480), CV_8UC1, (void*)dev->get_frame_data(rs::stream::infrared), Mat::AUTO_STEP);

    // Apply Histogram Equalization
    equalizeHist( ir, ir );
    applyColorMap(ir, ir, COLORMAP_JET);

    // Display the image in GUI
    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", ir);

    waitKey(0);

    return 0;
}
```

Compile and run the program from the terminal, with the following command:

```shell
g++ -std=c++11 IR_sample.cpp -lrealsense -lopencv_core -lopencv_omgproc -lopencv_contrib -o ir && ./ir
```

**Result :**

![IR_Image2](./resources/Image_IR.png)
