# Box dimensions calculation using multiple realsense camera

## Required python wrappers
OpenCV
LibRealSense
Numpy

## Aim
This sample demonstrates the ability to use the SDK for aligning multiple devices to a unified co-ordinate system in world to solve a simple task such as dimension calculation of a box. 

## Workflow
1. Place the calibration chessboard object into the field of view of all the realsense cameras. Update the chessboard parameters in the script in case a different size is chosen.                                 
2. Start the program.                                                                                                 
3. Allow calibration to occur and place the desired object ON the calibration object when the program asks for it. Make sure that the object to be measured is not bigger than the calibration object in length and width.            
4. The length, width and height of the bounding box of the object is then displayed in millimeters.                   
Note: To keep the demo simpler, the clipping of the usable point cloud is done based on the assumption that the object is placed ON the calibration object and the length and width is less than that of the calibration object. 


## Example Output
Once the calibration is done and the target object's dimensions are calculated, the application will open as many windows as the number of devices connected each displaying a color image along with an overlay of the calculated bounding box.
In the following example we've used two Intel® RealSense™ Depth Cameras D435 pointing at a common object placed on a 6 x 9 chessboard (checked-in with this demo folder).

![Screenshot](sampleOutput.png)
