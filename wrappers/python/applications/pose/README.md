![Pose6D](doc/pose6d-cup.gif)

[![image](https://img.shields.io/pypi/v/scikit-spatial.svg)](https://pypi.python.org/pypi/scikit-spatial)



# Introduction

This demo shows implementation of 6 degree of freedom (DoF) object detcetion and pose estimation based on RGB data of known object.


-   6D Pose of a Chess board  : estimates translation and rotation of the chess pattern
-   6D Pose of a Aruco marker : estimates translation and rotation of the aruco marker
-   6D Pose of a Known object : estimates translation and rotation of a knwon objects

The camera calibration parameters are known. If the camera position is also known this code could be used to control robots and automation processes. This code could be easily integrated in your robotics and video processing pipe line.

# 6 DoF Pose Estimation

-  Display Modes - different image data that could be extracted from the camera. 

Example Image   | Mode |  Explanations | 
:------------: |  :----------: | :----------: | 
![RGB](doc/chess.gif) | Chess Board |  RGB based chess board pose estimation | 
![d16](doc/cam_d_3.jpg) |   Aruco Marker | Aruco marker detection and pose estimation | 
![sc1](doc/pose6d-cup.gif) |   Known Object | cup detection and pose estimation |
![sc1](doc/pose6d-matchbox.gif) |   Known Object | a match box detection and pose estimation |

# Modules and License

For 6 DoF object pose estimation we are using code available from RobotAI ([https://wwww.robotai.info](https://www.robotai.info/))

## Installation Windows/Ubuntu

1. Install Pose6D executable from RobotAI

## Usage

1. Install Pose6D executable:

2. Connect to RealSense camera

3. Acquire dataset of the object you want to detect

4. Label the data using RobotAI labeling tool

5. Train AI model

6. Run Pose6D software to detect the object in real time

## Usage

Start Pose6D server and connect to it.
