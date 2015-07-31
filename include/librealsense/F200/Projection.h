#pragma once

#ifndef LIBREALSENSE_F200_PROJECTION_H
#define LIBREALSENSE_F200_PROJECTION_H

#include "libuvc/libuvc.h"

//@tofix better comments
namespace f200
{
    /// The method return the UV map of the depth image
    //Input: count           - number of points to be maped (can be used to map part of the image).
    //		 pos2d	           - array of 3D points in the the size of count
    //						     pos2d.z is the depth in mm units and pos2d.x and pos2d.y are the index of the pixels.
    //		 isUVunitsRelative - if true  - output UV units are in the range of 0-1 and relative to the image size
    //							 if false - output UV units are absolute.
    //Output: posc   - array of 3D points in the size of npoint pos2d.x and pos2d.y are the UV map of pixel (x,y)
    void MapDepthToColorCoordinates(uint32_t count, float * pos2d, float *posc, bool isUVunitsRelative = true);
    
    // The method return the UV map of the depth image
    //Input: width             - width of the image.
    //		 height			   - height of the image.
    //		 src	       - array of input depth in mm in the size of width*height.
    //		 isUVunitsRelative - if true  - output UV units are in the range of 0-1 and relative to the image size
    //							 if false - output UV units are absolute.
    //Output: dest          - array of output UV map should be allocated in the size of width*height*2.
    void MapDepthToColorCoordinates(uint32_t width, uint32_t height, uint16_t * src, float * dest, bool isUVunitsRelative = true);
    
    // The method convert the depth to coordinates in real world
    //Input: count           - number of points to be converted (can be used to convert part of the image).
    //		 pos2d	          - array of 3D points in the the size of count
    //						     pos2d.z is the depth in mm units and pos2d.x and pos2d.y are the index of the pixels.
    //Output: dest            - array of 3D points in the size of npoint pos2d.x pos2d.y pos2d.z are the coordinates in real world of pixel (x,y)
    void ProjectImageToRealWorld(uint32_t count, float * pos2d, float * dest);
    
    //The method convert the depth to coordinates in real world
    //Input: width             - width of the image.
    //		 height			   - height of the image.
    //		 src	           - array of input depth in mm in the size of width*height.
    //Output: dest         - array of output XYZ coordinates in real world of the corresponding to the input depth image should be allocated in the size of width*height*3.
    void ProjectImageToRealWorld(uint32_t width, uint32_t height, uint16_t * src, float * dest);
    
    
    // The method convert the depth in 1/32 mm units to mm
    //Input: d                 - depth in 1/32 mm units.
    //return: float            - depth in mm units.
    float DepthToMm(uint16_t d);
    
} // end namespace f200

#endif
