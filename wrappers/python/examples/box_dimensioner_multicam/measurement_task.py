##################################################################################################
##       License: Apache 2.0. See LICENSE file in root directory.		                      ####
##################################################################################################
##                  Box Dimensioner with multiple cameras: Helper files 					  ####
##################################################################################################

import pyrealsense2 as rs
import numpy as np
import cv2
from realsense_device_manager import post_process_depth_frame
from helper_functions import convert_depth_frame_to_pointcloud, get_clipped_pointcloud


def calculate_cumulative_pointcloud(frames_devices, calibration_info_devices, roi_2d, depth_threshold = 0.01):
	"""
 Calculate the cumulative pointcloud from the multiple devices
	Parameters:
	-----------
	frames_devices : dict
		The frames from the different devices
		keys: str
			Serial number of the device
		values: [frame]
			frame: rs.frame()
				The frameset obtained over the active pipeline from the realsense device
				
	calibration_info_devices : dict
		keys: str
			Serial number of the device
		values: [transformation_devices, intrinsics_devices]
			transformation_devices: Transformation object
					The transformation object containing the transformation information between the device and the world coordinate systems
			intrinsics_devices: rs.intrinscs
					The intrinsics of the depth_frame of the realsense device
					
	roi_2d : array
		The region of interest given in the following order [minX, maxX, minY, maxY]
		
	depth_threshold : double
		The threshold for the depth value (meters) in world-coordinates beyond which the point cloud information will not be used.
		Following the right-hand coordinate system, if the object is placed on the chessboard plane, the height of the object will increase along the negative Z-axis
	
	Return:
	----------
	point_cloud_cumulative : array
		The cumulative pointcloud from the multiple devices
	"""
	# Use a threshold of 5 centimeters from the chessboard as the area where useful points are found
	point_cloud_cumulative = np.array([-1, -1, -1]).transpose()
	for (device, frame) in frames_devices.items() :
		# Filter the depth_frame using the Temporal filter and get the corresponding pointcloud for each frame
		filtered_depth_frame = post_process_depth_frame(frame[rs.stream.depth], temporal_smooth_alpha=0.1, temporal_smooth_delta=80)	
		point_cloud = convert_depth_frame_to_pointcloud( np.asarray( filtered_depth_frame.get_data()), calibration_info_devices[device][1][rs.stream.depth])
		point_cloud = np.asanyarray(point_cloud)

		# Get the point cloud in the world-coordinates using the transformation
		point_cloud = calibration_info_devices[device][0].apply_transformation(point_cloud)

		# Filter the point cloud based on the depth of the object
		# The object placed has its height in the negative direction of z-axis due to the right-hand coordinate system
		point_cloud = get_clipped_pointcloud(point_cloud, roi_2d)
		point_cloud = point_cloud[:,point_cloud[2,:]<-depth_threshold]
		point_cloud_cumulative = np.column_stack( ( point_cloud_cumulative, point_cloud ) )
	point_cloud_cumulative = np.delete(point_cloud_cumulative, 0, 1)
	return point_cloud_cumulative




def calculate_boundingbox_points(point_cloud, calibration_info_devices, depth_threshold = 0.01):
	"""
	Calculate the top and bottom bounding box corner points for the point cloud in the image coordinates of the color imager of the realsense device
	
	Parameters:
	-----------
	point_cloud : ndarray
		The (3 x N) array containing the pointcloud information
		
	calibration_info_devices : dict
		keys: str
			Serial number of the device
		values: [transformation_devices, intrinsics_devices, extrinsics_devices]
			transformation_devices: Transformation object
					The transformation object containing the transformation information between the device and the world coordinate systems
			intrinsics_devices: rs.intrinscs
					The intrinsics of the depth_frame of the realsense device
			extrinsics_devices: rs.extrinsics
					The extrinsics between the depth imager 1 and the color imager of the realsense device
					
	depth_threshold : double
		The threshold for the depth value (meters) in world-coordinates beyond which the point cloud information will not be used
		Following the right-hand coordinate system, if the object is placed on the chessboard plane, the height of the object will increase along the negative Z-axis
		
	Return:
	----------
	bounding_box_points_color_image : dict
		The bounding box corner points in the image coordinate system for the color imager
		keys: str
				Serial number of the device
			values: [points]
				points: list
					The (8x2) list of the upper corner points stacked above the lower corner points 
					
	length : double
		The length of the bounding box calculated in the world coordinates of the pointcloud
		
	width : double
		The width of the bounding box calculated in the world coordinates of the pointcloud
		
	height : double
		The height of the bounding box calculated in the world coordinates of the pointcloud
	"""
	# Calculate the dimensions of the filtered and summed up point cloud
	# Some dirty array manipulations are gonna follow
	if point_cloud.shape[1] > 500:
		# Get the bounding box in 2D using the X and Y coordinates
		coord = np.c_[point_cloud[0,:], point_cloud[1,:]].astype('float32')
		min_area_rectangle = cv2.minAreaRect(coord)
		bounding_box_world_2d = cv2.boxPoints(min_area_rectangle)
		# Caculate the height of the pointcloud
		height = max(point_cloud[2,:]) - min(point_cloud[2,:]) + depth_threshold

		# Get the upper and lower bounding box corner points in 3D
		height_array = np.array([[-height], [-height], [-height], [-height], [0], [0], [0], [0]])
		bounding_box_world_3d = np.column_stack((np.row_stack((bounding_box_world_2d,bounding_box_world_2d)), height_array))

		# Get the bounding box points in the image coordinates
		bounding_box_points_color_image={}
		for (device, calibration_info) in calibration_info_devices.items():
			# Transform the bounding box corner points to the device coordinates
			bounding_box_device_3d = calibration_info[0].inverse().apply_transformation(bounding_box_world_3d.transpose())
			
			# Obtain the image coordinates in the color imager using the bounding box 3D corner points in the device coordinates
			color_pixel=[]
			bounding_box_device_3d = bounding_box_device_3d.transpose().tolist()
			for bounding_box_point in bounding_box_device_3d: 
				bounding_box_color_image_point = rs.rs2_transform_point_to_point(calibration_info[2], bounding_box_point)			
				color_pixel.append(rs.rs2_project_point_to_pixel(calibration_info[1][rs.stream.color], bounding_box_color_image_point))
			
			bounding_box_points_color_image[device] = np.row_stack( color_pixel )
		return bounding_box_points_color_image, min_area_rectangle[1][0], min_area_rectangle[1][1], height
	else : 
		return {},0,0,0



def visualise_measurements(frames_devices, bounding_box_points_devices, length, width, height):
	"""
 Calculate the cumulative pointcloud from the multiple devices
	
	Parameters:
	-----------
	frames_devices : dict
		The frames from the different devices
		keys: str
			Serial number of the device
		values: [frame]
			frame: rs.frame()
				The frameset obtained over the active pipeline from the realsense device
				
	bounding_box_points_color_image : dict
		The bounding box corner points in the image coordinate system for the color imager
		keys: str
				Serial number of the device
			values: [points]
				points: list
					The (8x2) list of the upper corner points stacked above the lower corner points 
					
	length : double
		The length of the bounding box calculated in the world coordinates of the pointcloud
		
	width : double
		The width of the bounding box calculated in the world coordinates of the pointcloud
		
	height : double
		The height of the bounding box calculated in the world coordinates of the pointcloud
	"""
	for (device, frame) in frames_devices.items():
		color_image = np.asarray(frame[rs.stream.color].get_data())
		if (length != 0 and width !=0 and height != 0):
			bounding_box_points_device_upper = bounding_box_points_devices[device][0:4,:]
			bounding_box_points_device_lower = bounding_box_points_devices[device][4:8,:]
			box_info = "Length, Width, Height (mm): " + str(int(length*1000)) + ", " + str(int(width*1000)) + ", " + str(int(height*1000))

			# Draw the box as an overlay on the color image		
			bounding_box_points_device_upper = tuple(map(tuple,bounding_box_points_device_upper.astype(int)))
			for i in range(len(bounding_box_points_device_upper)):	
				cv2.line(color_image, bounding_box_points_device_upper[i], bounding_box_points_device_upper[(i+1)%4], (0,255,0), 4)

			bounding_box_points_device_lower = tuple(map(tuple,bounding_box_points_device_lower.astype(int)))
			for i in range(len(bounding_box_points_device_upper)):	
				cv2.line(color_image, bounding_box_points_device_lower[i], bounding_box_points_device_lower[(i+1)%4], (0,255,0), 1)
				
			cv2.line(color_image, bounding_box_points_device_upper[0], bounding_box_points_device_lower[0], (0,255,0), 1)
			cv2.line(color_image, bounding_box_points_device_upper[1], bounding_box_points_device_lower[1], (0,255,0), 1)
			cv2.line(color_image, bounding_box_points_device_upper[2], bounding_box_points_device_lower[2], (0,255,0), 1)
			cv2.line(color_image, bounding_box_points_device_upper[3], bounding_box_points_device_lower[3], (0,255,0), 1)
			cv2.putText(color_image, box_info, (50,50), cv2.FONT_HERSHEY_PLAIN, 2, (0,255,0) )
			
		# Visualise the results
		cv2.imshow('Color image from RealSense Device Nr: ' + device, color_image)
		cv2.waitKey(1)
