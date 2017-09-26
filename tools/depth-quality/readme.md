# Depth Quality Tool

<p align="center"><img src="res/depth_quality_glimpse.gif" /></p>

## Overview

This application is designed to provide the end-user with quantitative metrics that characterize the quality of the produced depth data.
You should be able to easily get and interpret several of stable metrics, see the effect of the camera controls and save the data for offline analysis.

## Quick Start
* Position the depth camera within a range of 0.5 -3 meter from a flat non-reflective surface.
* Aim the camera to the target and hold it steady for several seconds till the tool is able to recognize the surface (the yellow grid in 3D view).
* Adjust the camera using the "Angle" metric to minimize the skew and make it as  perpendicular to the surface as possible.
* Inspect the calculated depth quality metrics, expand metric properties to get more in-depth info.

## Features
* 2D/3D Depth View
* Plane Fitting - using reconstructed surface
* User-defined Region of Interest
* Depth Quality metrics:
  * Depth average error
  * Standard Deviation
  * Fill-Rate
  * Outliers
  * Distance to target
* Export metrics and device configuration
* Depth Sensor controls
