###########################################################################################################################
##                      License: Apache 2.0. See LICENSE file in root directory.                                         ##
###########################################################################################################################
##                  Simple Box Dimensioner with multiple cameras: Main demo file                                         ##
###########################################################################################################################
## Workflow description:                                                                                                 ##
## 1. Place the calibration chessboard object into the field of view of all the realsense cameras.                       ##
##    Update the chessboard parameters in the script in case a different size is chosen.                                 ##
## 2. Start the program.                                                                                                 ##
## 3. Allow calibration to occur and place the desired object ON the calibration object when the program asks for it.    ##
##    Make sure that the object to be measured is not bigger than the calibration object in length and width.            ##
## 4. The length, width and height of the bounding box of the object is then displayed in millimeters.                   ##
###########################################################################################################################

# Import RealSense, OpenCV and NumPy
import pyrealsense2 as rs
import cv2
import numpy as np

# Import helper functions and classes written to wrap the RealSense, OpenCV and Kabsch Calibration usage
from collections import defaultdict
from realsense_device_manager import DeviceManager
from calibration_kabsch import PoseEstimation
from helper_functions import get_boundary_corners_2D
from measurement_task import calculate_boundingbox_points, calculate_cumulative_pointcloud, visualise_measurements

def run_demo():
	
	# Define some constants 
	L515_resolution_width = 1024 # pixels
	L515_resolution_height = 768 # pixels
	L515_frame_rate = 30

	resolution_width = 1280 # pixels
	resolution_height = 720 # pixels
	frame_rate = 15  # fps

	dispose_frames_for_stablisation = 30  # frames
	
	chessboard_width = 6 # squares
	chessboard_height = 9 	# squares
	square_size = 0.0253 # meters

	try:
		# Enable the streams from all the intel realsense devices
		L515_rs_config = rs.config()
		L515_rs_config.enable_stream(rs.stream.depth, L515_resolution_width, L515_resolution_height, rs.format.z16, L515_frame_rate)
		L515_rs_config.enable_stream(rs.stream.infrared, 0, L515_resolution_width, L515_resolution_height, rs.format.y8, L515_frame_rate)
		L515_rs_config.enable_stream(rs.stream.color, resolution_width, resolution_height, rs.format.bgr8, frame_rate)

		rs_config = rs.config()
		rs_config.enable_stream(rs.stream.depth, resolution_width, resolution_height, rs.format.z16, frame_rate)
		rs_config.enable_stream(rs.stream.infrared, 1, resolution_width, resolution_height, rs.format.y8, frame_rate)
		rs_config.enable_stream(rs.stream.color, resolution_width, resolution_height, rs.format.bgr8, frame_rate)

		# Use the device manager class to enable the devices and get the frames
		device_manager = DeviceManager(rs.context(), rs_config, L515_rs_config)
		device_manager.enable_all_devices()
		
		# Allow some frames for the auto-exposure controller to stablise
		for frame in range(dispose_frames_for_stablisation):
			frames = device_manager.poll_frames()

		assert( len(device_manager._available_devices) > 0 )
		"""
		1: Calibration
		Calibrate all the available devices to the world co-ordinates.
		For this purpose, a chessboard printout for use with opencv based calibration process is needed.
		
		"""
		# Get the intrinsics of the realsense device 
		intrinsics_devices = device_manager.get_device_intrinsics(frames)
		
                # Set the chessboard parameters for calibration 
		chessboard_params = [chessboard_height, chessboard_width, square_size] 
		
		# Estimate the pose of the chessboard in the world coordinate using the Kabsch Method
		calibrated_device_count = 0
		while calibrated_device_count < len(device_manager._available_devices):
			frames = device_manager.poll_frames()
			pose_estimator = PoseEstimation(frames, intrinsics_devices, chessboard_params)
			transformation_result_kabsch  = pose_estimator.perform_pose_estimation()
			object_point = pose_estimator.get_chessboard_corners_in3d()
			calibrated_device_count = 0
			for device_info in device_manager._available_devices:
				device = device_info[0]
				if not transformation_result_kabsch[device][0]:
					print("Place the chessboard on the plane where the object needs to be detected..")
				else:
					calibrated_device_count += 1

		# Save the transformation object for all devices in an array to use for measurements
		transformation_devices={}
		chessboard_points_cumulative_3d = np.array([-1,-1,-1]).transpose()
		for device_info in device_manager._available_devices:
			device = device_info[0]
			transformation_devices[device] = transformation_result_kabsch[device][1].inverse()
			points3D = object_point[device][2][:,object_point[device][3]]
			points3D = transformation_devices[device].apply_transformation(points3D)
			chessboard_points_cumulative_3d = np.column_stack( (chessboard_points_cumulative_3d,points3D) )

		# Extract the bounds between which the object's dimensions are needed
		# It is necessary for this demo that the object's length and breath is smaller than that of the chessboard
		chessboard_points_cumulative_3d = np.delete(chessboard_points_cumulative_3d, 0, 1)
		roi_2D = get_boundary_corners_2D(chessboard_points_cumulative_3d)

		print("Calibration completed... \nPlace the box in the field of view of the devices...")


		"""
                2: Measurement and display
                Measure the dimension of the object using depth maps from multiple RealSense devices
                The information from Phase 1 will be used here

                """

		# Enable the emitter of the devices
		device_manager.enable_emitter(True)

		# Load the JSON settings file in order to enable High Accuracy preset for the realsense
		device_manager.load_settings_json("./HighResHighAccuracyPreset.json")

		# Get the extrinsics of the device to be used later
		extrinsics_devices = device_manager.get_depth_to_color_extrinsics(frames)

		# Get the calibration info as a dictionary to help with display of the measurements onto the color image instead of infra red image
		calibration_info_devices = defaultdict(list)
		for calibration_info in (transformation_devices, intrinsics_devices, extrinsics_devices):
			for key, value in calibration_info.items():
				calibration_info_devices[key].append(value)

		# Continue acquisition until terminated with Ctrl+C by the user
		while 1:
			 # Get the frames from all the devices
				frames_devices = device_manager.poll_frames()

				# Calculate the pointcloud using the depth frames from all the devices
				point_cloud = calculate_cumulative_pointcloud(frames_devices, calibration_info_devices, roi_2D)

				# Get the bounding box for the pointcloud in image coordinates of the color imager
				bounding_box_points_color_image, length, width, height = calculate_boundingbox_points(point_cloud, calibration_info_devices )

				# Draw the bounding box points on the color image and visualise the results
				visualise_measurements(frames_devices, bounding_box_points_color_image, length, width, height)

	except KeyboardInterrupt:
		print("The program was interupted by the user. Closing the program...")
	
	finally:
		device_manager.disable_streams()
		cv2.destroyAllWindows()
	
	
if __name__ == "__main__":
	run_demo()
