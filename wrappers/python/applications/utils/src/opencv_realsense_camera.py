
'''
OpenCV like wrapper for Real Sense Camera

==================

Allows to read, display store video and images of RGB - Depth combinations in different formats.  
Can extract left and right IR images.
Aligns RGB and Depth data.
Can save data as mp4 or single images.
Can control laser power, exposure and other parameters.

Usage:
    python opencv_realsense_camera.py 
    will run the camera and open the image window with live stream.
    Use keys outlines in test() function to switch different modes

    Press 'd' to show different display options
    Press 'e' to show exposure control    
    Press 'p' to show projector control      
    Press 'g' to show gain control     
    Press 's' to save the current image
    Press 't' to save the left and right images in separate files
    Press 'r' to start recording and one more time 'r' to stop video recording
    Press 'a' to show advanced features    
    
                                        
Environment : 
    ..\\Envs\\barcode

Install : 
    pip install pyrealsense2-2.56.0.7981-cp310-cp310-win_amd64.whl

'''
import os
import pyrealsense2 as rs
import numpy as np
import cv2 as cv
import time

#%% Draw

def draw_str(dst, target, s):
    x, y = target
    dst = cv.putText(dst, s, (x+1, y+1), cv.FONT_HERSHEY_PLAIN, 1.0, (0, 0, 0), thickness = 2, lineType=cv.LINE_AA)
    dst = cv.putText(dst, s, (x, y), cv.FONT_HERSHEY_PLAIN, 1.0, (255, 255, 255), lineType=cv.LINE_AA)
    return dst

#%% Helper
DS5_product_ids = ["0AD1", "0AD2", "0AD3", "0AD4", "0AD5", "0AF6", "0AFE", "0AFF", "0B00", "0B01", "0B03", "0B07", "0B3A", "0B5C", "0B5B"]

def find_device_that_supports_advanced_mode() :
    ctx = rs.context()
    ds5_dev = rs.device()
    devices = ctx.query_devices()
    for dev in devices:
        if dev.supports(rs.camera_info.product_id) and str(dev.get_info(rs.camera_info.product_id)) in DS5_product_ids:
            if dev.supports(rs.camera_info.name):
                print("Found device that supports advanced mode:", dev.get_info(rs.camera_info.name))
            return dev
    raise Exception("No D400 product line device that supports advanced mode was found")


#%% Main
class RealSense(object):
    def __init__(self,  mode = 'rgb', use_ir = True, frame_size = None):
        
        self.frame_size     = (1280, 720) if frame_size is None else frame_size  #frame_size #(1280, 720)#(640,480)
        self.display_mode   = 'rgb' if mode is None else mode 
        self.use_ir         = False if use_ir is None else use_ir
        self.use_projector  = False
        self.use_advanced   = False  # advanced mode is enabled
        self.control_mode   = 'no controls' 
        self.DISPLAY_MODES  = ['rgb','rgd','ddd','d16','gdd','scl','sc2','dep','iid','ii2','iig','iir','gd','ggd']

        # noise measurement
        self.img_int_mean   = None
        self.img_int_std    = None
        self.use_measure    = False   
        self.rect           = None     

        # Configure depth and color streams
        self.pipeline       = rs.pipeline()
        self.config         = rs.config()

        #  Get device product line for setting a supporting resolution
        device_name         = self.get_device_name()
        self.set_frame_size(device_name)

        # start streaming
        self.set_start_streaming()

        # set advanced mode - disparity in pixels
        self.set_advanced_mode()
        self.advance_mode = None

        # # turn emitter on-off
        self.has_projector       = device_name.find('D455') > 0   or device_name.find('D555') > 0
        self.switch_projector()   

        # Depth controls to defaults
        self.set_exposure()
        self.set_gain()
        self.set_laser_power()        

        # Create an align object
        # rs.align allows us to perform alignment of depth frames to others frames
        # The "align_to" is the stream type to which we plan to align depth frames.
        align_to        = rs.stream.color
        self.align      = rs.align(align_to)

        # output support
        self.output_range = [0,255]  # extract range to map 16 bit to 8

        # record video
        self.vout       = None
        self.record_on  = False # toggle recording
        self.count      = 0

    def render(self, dst):
        pass

    def get_device_name(self):
        "find device name"
        device_name         = ''
        pipeline_wrapper    = rs.pipeline_wrapper(self.pipeline)
        try: # 545
            pipeline_profile    = self.config.resolve(pipeline_wrapper)
            device              = pipeline_profile.get_device()
            device_product_line = str(device.get_info(rs.camera_info.product_line))
            device_name         = device.get_info(rs.camera_info.name)
            print('Device name : ', device_name)
            print('Device product line : ', device_product_line)
        except Exception as e:
            print('Real Sense new version - possibly will require a new driver version')
            print(e)

        return device_name
    
    def set_frame_size(self, device_name):
        "device dependent data"
        if  device_name.find('D585') > 0 or device_name.find('D555') > 0:
            print(f'Configured for {device_name}')     
            self.frame_size = (1280, 720)  

        print(f'Frame size  : {self.frame_size[0]} x {self.frame_size[1]}')

    def set_start_streaming(self):
        "start stremaing"
        self.config.enable_stream(rs.stream.depth, self.frame_size[0], self.frame_size[1], rs.format.z16, 30)
        self.config.enable_stream(rs.stream.color, self.frame_size[0], self.frame_size[1], rs.format.bgr8, 30)
        
        if self.use_ir:
            self.config.enable_stream(rs.stream.infrared, 1)
            self.config.enable_stream(rs.stream.infrared, 2)
            print('IR is enabled')
        else:
            print('IR is disabled')                 


        # Start streaming
        profile             = self.pipeline.start(self.config)

        # Getting the depth sensor's depth scale (see rs-align example for explanation)
        self.depth_sensor   = profile.get_device().first_depth_sensor()
        #depth_scale         = self.depth_sensor.get_depth_scale()
        #print("Depth Scale is: " , depth_scale)        

    def set_advanced_mode(self):
        "enable camera advanced mode"
        if not self.use_advanced:
            return
        
        try:
            dev         = find_device_that_supports_advanced_mode()
            advnc_mode  = rs.rs400_advanced_mode(dev)
            print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")

            # Loop until we successfully enable advanced mode
            while not advnc_mode.is_enabled():
                print("Trying to enable advanced mode...")
                advnc_mode.toggle_advanced_mode(True)
                # At this point the device will disconnect and re-connect.
                print("Sleeping for 5 seconds...")
                time.sleep(5)
                # The 'dev' object will become invalid and we need to initialize it again
                dev = find_device_that_supports_advanced_mode()
                advnc_mode = rs.rs400_advanced_mode(dev)
                print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")

            # Get each control's current value
            print("Depth Control: \n", advnc_mode.get_depth_control())
            print("RSM: \n", advnc_mode.get_rsm())
            print("RAU Support Vector Control: \n", advnc_mode.get_rau_support_vector_control())
            print("Color Control: \n", advnc_mode.get_color_control())
            print("RAU Thresholds Control: \n", advnc_mode.get_rau_thresholds_control())
            print("SLO Color Thresholds Control: \n", advnc_mode.get_slo_color_thresholds_control())
            print("SLO Penalty Control: \n", advnc_mode.get_slo_penalty_control())
            print("HDAD: \n", advnc_mode.get_hdad())
            print("Color Correction: \n", advnc_mode.get_color_correction())
            print("Depth Table: \n", advnc_mode.get_depth_table())
            print("Auto Exposure Control: \n", advnc_mode.get_ae_control())
            print("Census: \n", advnc_mode.get_census())

        except Exception as e:
            print(e)
            return   

        #UD - enable disparity mode output
        depth_table = advnc_mode.get_depth_table()
        depth_table.disparityMode = 1   # 0-depth,1-disparity
        advnc_mode.set_depth_table(depth_table)
        print("Depth Table: \n", advnc_mode.get_depth_table()) # confirm the settings


        # #UD - Simulator settings
        hdad = advnc_mode.get_hdad()
        hdad.ignoreSAD = 1
        advnc_mode.set_hdad(hdad)
        print("HDAD: \n", advnc_mode.get_hdad())

        color_cntrl = advnc_mode.get_color_control()
        color_cntrl.disableSADColor = 1
        color_cntrl.disableRAUColor = 1
        advnc_mode.set_color_control(color_cntrl)
        print("Color Correction: \n", advnc_mode.get_color_control())

        # no difference
        # rau_cntrl = advnc_mode.get_rau_support_vector_control()
        # rau_cntrl.minWEsum = 1
        # rau_cntrl.minNSsum = 1
        # advnc_mode.set_rau_support_vector_control(rau_cntrl)
        # print("RAU Support Vector Control: \n", advnc_mode.get_color_control())        

        rsm = advnc_mode.get_rsm()
        rsm.rsmBypass = 1   
        advnc_mode.set_rsm(rsm)
        print("RSM: \n", advnc_mode.get_rsm())

        depth_cntrl = advnc_mode.get_depth_control()
        depth_cntrl.scoreThreshA = 0
        depth_cntrl.deepSeaSecondPeakThreshold = 50
        advnc_mode.set_depth_control(depth_cntrl)
        print("Depth Control: \n", advnc_mode.get_depth_control())

        slo_cntrl = advnc_mode.get_slo_penalty_control()
        slo_cntrl.sloK1Penalty = 400
        slo_cntrl.sloK2Penalty = 511
        advnc_mode.set_slo_penalty_control(slo_cntrl)
        print("SLO Penalty Control: \n", advnc_mode.get_slo_penalty_control())

        #self.depth_sensor = dev
        #return dev         

    def set_exposure(self, exposure_value = None):
        "set exposure to the correct values"
        
        if self.depth_sensor.supports(rs.option.exposure):
            range           = self.depth_sensor.get_option_range(rs.option.exposure)
            exposure_value  = exposure_value if exposure_value is not None else range.default 
            exposure_value  = exposure_value if exposure_value > range.min else range.min 
            exposure_value  = exposure_value if exposure_value < range.max else range.max 

            self.depth_sensor.set_option(rs.option.exposure, int(exposure_value))
            print(f'Exposure is : {exposure_value}')
        else:
            print('Exposure has no support')

    def set_gain(self,gain_value = None):
        "set gain to the correct values"
        if self.depth_sensor.supports(rs.option.gain):
            range           = self.depth_sensor.get_option_range(rs.option.gain)
            gain_value  = gain_value if gain_value is not None else range.default 
            gain_value  = gain_value if gain_value > range.min else range.min 
            gain_value  = gain_value if gain_value < range.max else range.max 

            self.depth_sensor.set_option(rs.option.gain, int(gain_value))
            print(f'Gain is : {gain_value}')
        else:
            print('Gain has no support')    

    def set_output_range(self, range_value = 0):
        "maps 16 bit to 8"
        range_value             = range_value * 255
        self.output_range[0]    = range_value
        self.output_range[1]    = range_value + 255
        print(f'Output range is set to min {self.output_range[0]} and max {self.output_range[1]}')  

    def get_baseline(self):
        "returns camera baseline"
        B = self.depth_sensor.get_option(rs.option.stereo_baseline)
        print(f'Baseline is : {B} mm')
        return B
    
    def get_focal_length(self):
        "intrinsic parameters and returns focal length"
        pipeline_wrapper    = rs.pipeline_wrapper(self.pipeline)
        pipeline_profile    = self.config.resolve(pipeline_wrapper)        
        intr                = pipeline_profile.get_stream(rs.stream.depth).as_video_stream_profile().get_intrinsics()
        print(f'Intrinsics Fx is : {intr.fx} ')
        return intr.fx
    
    def get_camera_intrinsics(self):
        "intrinsic parameters of the camera"
        pipeline_wrapper    = rs.pipeline_wrapper(self.pipeline)
        pipeline_profile    = self.config.resolve(pipeline_wrapper)        
        intr                = pipeline_profile.get_stream(rs.stream.depth).as_video_stream_profile().get_intrinsics()
        #print(f'Intrinsics Fx is : {intr.fx} ')
        print(intr)
        return intr.fx    
    
    def get_bf(self):
        "read baseline and focal length for inverse depth compute"
        b = self.get_baseline()
        f = self.get_focal_length()
        print(f'Total BF is : {b*f} ')
        return b*f
    
    def get_camera_params(self, value_in = 0):
        "whoch camera params toi show"
        if value_in == 0:
            self.get_bf()
        elif value_in == 1:
            self.get_camera_intrinsics()

    def set_laser_power(self, laser_power_value = None):
        "set laser power to the correct values"
        
        if self.depth_sensor.supports(rs.option.laser_power):
            range           = self.depth_sensor.get_option_range(rs.option.laser_power)
            laser_power_value  = laser_power_value if laser_power_value is not None else range.default 
            laser_power_value  = laser_power_value if laser_power_value > range.min else range.min 
            laser_power_value  = laser_power_value if laser_power_value < range.max else range.max 

            self.depth_sensor.set_option(rs.option.laser_power, int(laser_power_value))
            print(f'Laser power is : {laser_power_value}')
        else:
            print('Laser power has no support')       

    def switch_projector(self):
        "switch projectpr on-off"        
        if not self.has_projector:
            print('Camera is without projector')
        else:
            #if self.use_projector is False:
            self.depth_sensor.set_option(rs.option.emitter_always_on, self.use_projector)
            self.depth_sensor.set_option(rs.option.emitter_enabled, self.use_projector)
                
            time.sleep(0.1) # wait for camera on - off
            print('Camera projector : %s' %str(self.use_projector)) 

    def switch_disparity(self):
        "switch disparity on"        

        if self.advance_mode is None:
            dev         = find_device_that_supports_advanced_mode()
            advnc_mode  = rs.rs400_advanced_mode(dev)
            print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")

            # Loop until we successfully enable advanced mode
            while not advnc_mode.is_enabled():
                print("Trying to enable advanced mode...")
                advnc_mode.toggle_advanced_mode(True)
                # At this point the device will disconnect and re-connect.
                print("Sleeping for 5 seconds...")
                time.sleep(5)
                # The 'dev' object will become invalid and we need to initialize it again
                dev = find_device_that_supports_advanced_mode()
                advnc_mode = rs.rs400_advanced_mode(dev)
                print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")  

            self.advance_mode       = advnc_mode 

        depth_table                 = self.advance_mode.get_depth_table()
        depth_table.disparityMode   = 1 - depth_table.disparityMode  # 0-depth,1-disparity - switch
        self.advance_mode.set_depth_table(depth_table)
        print("Depth Table: \n", self.advance_mode.get_depth_table()) # confirm the settings                     
            
    def set_display_mode(self, mode = 'rgb'):
        "changes display mode by umber or by string"
        
        if isinstance(mode,int): # integer
            mode  = mode % len(self.DISPLAY_MODES)
            mode  = self.DISPLAY_MODES[mode]

        if not(mode in self.DISPLAY_MODES):
             print(f'Not supported mode = {mode}')
               
        self.display_mode = mode  
        print(f'Current mode {mode}')

    def set_controls(self, value_in = 0):
        "implements differnt controls according to the selected control mode. Input is an integer from 0-9"
        if self.control_mode == 'display':
            self.set_display_mode(value_in)

        elif self.control_mode == 'exposure':
            self.set_exposure(value_in*2000)

        elif self.control_mode == 'gain':
            self.set_gain(value_in*10)      

        elif self.control_mode == 'projector':
            self.use_projector = value_in == 1
            self.switch_projector()    

        elif self.control_mode == 'disparity':     
            self.switch_disparity()   

        elif self.control_mode == 'range':     
            self.set_output_range(value_in) 

        elif self.control_mode == 'focal':     
            self.get_camera_params(value_in)                           
        else:
            pass        

    def convert_depth_to_disparity(self, img_depth):
        "from GIL"
        focal_len           = 175.910019
        baseline            = 94.773
        #replacementDepth    = focal_len *  baseline / (RectScaledInfra1.x - (maxLoc.x + RectScaledInfra2.x));
        img_disparity       = img_depth.copy()
        valid               = img_depth > 0
        img_disparity[valid]= focal_len*baseline/img_depth[valid]*32
        return img_disparity

    def measure_noise(self, img):
        "makes integration over ROI"
        x0, y0, x1, y1  = self.rect
        if len(img.shape) < 3:
            img_roi         = img[y0:y1,x0:x1].astype(np.float32)
        else: # protect from rgb display
            img_roi         = img[y0:y1,x0:x1,0].astype(np.float32)

        if self.img_int_mean is None:
            self.img_int_mean = img_roi
            self.img_int_std  = np.zeros_like(img_roi)
        elif self.img_int_mean.shape[1] != img_roi.shape[1]: # image display is changed
            self.img_int_mean = None
            return 0
        
        valid_bool        = img_roi > 0
        #valid_num         = valid_bool.sum()
        #nr,nc             = img_roi.shape[:2]

        self.img_int_mean += 0.1*(img_roi - self.img_int_mean)
        self.img_int_std  += 0.1*(np.abs(img_roi - self.img_int_mean) - self.img_int_std)

        err_std_valid      = self.img_int_std.copy()
        #err_std_valid[~valid_bool]    = 100
        err_std            = err_std_valid[valid_bool].mean()        

        return err_std

    def create_output_image(self, depth_image, color_image, irl_image, irr_image):
        "defines the output image"

        if self.display_mode == 'rgb':
            image_out       = color_image
        elif self.display_mode == 'ddd':
            # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
            depth_colormap  = cv.applyColorMap(depth_scaled, cv.COLORMAP_JET)            
            image_out       = depth_scaled
        elif self.display_mode == 'rgd':
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
            image_out       = np.concatenate((color_image[:,:,:2], depth_scaled[:,:,np.newaxis] ), axis = 2)
        elif self.display_mode == 'gd':
            gray_image      = cv.cvtColor(color_image, cv.COLOR_RGB2GRAY)
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
            image_out       = np.concatenate((gray_image, depth_scaled ), axis = 1)
        elif self.display_mode == 'ggd':
            gray_image      = cv.cvtColor(color_image, cv.COLOR_RGB2GRAY)
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
            image_out       = np.stack((gray_image, gray_image, depth_scaled ), axis = 2)            
        elif self.display_mode == 'gdd':
            gray_image      = cv.cvtColor(color_image, cv.COLOR_RGB2GRAY)
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
            image_out       = np.stack((gray_image, depth_scaled, depth_scaled ), axis = 2) 
        elif self.display_mode == 'scl':
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.05)
            image_out       = cv.applyColorMap(depth_scaled, cv.COLORMAP_JET)    
        elif self.display_mode == 'sc2':
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.1)
            image_out       = cv.applyColorMap(depth_scaled, cv.COLORMAP_JET)    
        elif self.display_mode == 'ii2':
            image_out       = np.concatenate((irl_image, irr_image), axis = 1)  
        elif self.display_mode == 'iid':
            #print(f'Depth {depth_image.min()} - {depth_image.max()}')
            depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.1)
            image_out       = np.stack((irl_image, irr_image, depth_scaled), axis = 2)  
            #image_out       = np.concatenate((irl_image, depth_scaled), axis = 1) 
        elif self.display_mode == 'd16':
            image_out       = np.stack((irl_image.astype(np.uint16), irr_image.astype(np.uint16), depth_image), axis = 2)              
        elif self.display_mode == 'iig':
            image_out       = np.stack((irl_image, irr_image, color_image[:,:,1]), axis = 2)                  
        elif self.display_mode == 'iir':
            image_out       = np.stack((irl_image, irr_image, color_image[:,:,0]), axis = 2) 
            #image_out       = np.concatenate((irl_image, color_image[:,:,0]), axis = 1) 
        elif self.display_mode == 'dep':
            image_out       = depth_image.astype(np.float32)
            image_out       = image_out - self.output_range[0]
            image_out[image_out < 0]   = 0
            image_out[image_out > 255] = 255

            
            #image_out       = depth_image / 32 * 4  # 10 for scaling  
            #image_out = self.convert_depth_to_disparity(depth_image)             
        return image_out        

    def read(self, dst=None):
        "with frame alignments and color space transformations"
        #self.use_projector = not self.use_projector # testing
        w, h                = self.frame_size

        # Wait for a coherent pair of frames: depth and color
        frames              = self.pipeline.wait_for_frames()
        # Align the depth frame to color frame
        aligned_frames      = self.align.process(frames)

        # Get aligned frames
        depth_frame         = aligned_frames.get_depth_frame() # aligned_depth_frame is a 640x480 depth image
        color_frame         = aligned_frames.get_color_frame()
        if not depth_frame or not color_frame:
            return False, None

        # Convert images to numpy arrays
        depth_image         = np.asanyarray(depth_frame.get_data())
        color_image         = np.asanyarray(color_frame.get_data())
        #color_image = cv.cvtColor(depth_image, cv.COLOR_GRAY2RGB)
        #depth_image = cv.cvtColor(color_image, cv.COLOR_RGB2GRAY)

        # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
        depth_scaled        = cv.convertScaleAbs(depth_image, alpha=0.03)
        depth_colormap      = cv.applyColorMap(depth_scaled, cv.COLORMAP_JET)

        depth_colormap_dim = depth_colormap.shape
        color_colormap_dim = color_image.shape

        #If depth and color resolutions are different, resize color image to match depth image for display
        if depth_colormap_dim != color_colormap_dim:
            raise ValueError('depth and image size missmatch')
            #color_image = cv.resize(color_image, dsize=(depth_colormap_dim[1], depth_colormap_dim[0]), interpolation=cv.INTER_AREA)
            #images = np.hstack((resized_color_image, depth_colormap))
        # else:
        #     images = np.hstack((color_image, depth_colormap))

        if self.use_ir:
            ir_left     = aligned_frames.get_infrared_frame(1)
            irl_image   = np.asanyarray(ir_left.get_data())
            ir_right    = aligned_frames.get_infrared_frame(2)
            irr_image   = np.asanyarray(ir_right.get_data())
        else:
            #print('Enable IR use at the start. use_ir = True')    
            irl_image   = color_image[:,:,0]
            irr_image   = color_image[:,:,1]
            image_out   = color_image            

        image_out =  self.create_output_image(depth_image, color_image, irl_image, irr_image)                
        return True, image_out

    def read_not_aligned(self, dst=None):
        "color and depth are not aligned"
        w, h = self.frame_size

        # Wait for a coherent pair of frames: depth and color
        frames = self.pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()
        if not depth_frame or not color_frame:
            return False, None

        # Convert images to numpy arrays
        depth_image     = np.asanyarray(depth_frame.get_data())
        color_image     = np.asanyarray(color_frame.get_data())
        #color_image = cv.cvtColor(depth_image, cv.COLOR_GRAY2RGB)
        #depth_image = cv.cvtColor(color_image, cv.COLOR_RGB2GRAY)

        # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
        depth_scaled    = cv.convertScaleAbs(depth_image, alpha=0.03)
        depth_colormap  = cv.applyColorMap(depth_scaled, cv.COLORMAP_JET)

        depth_colormap_dim = depth_colormap.shape
        color_colormap_dim = color_image.shape

        image_out  = depth_image 
        return True, image_out

    def isOpened(self):
        "OpenCV compatability"
        return True
    
    def save_image(self, frame):
        fn = '.\\image_%s_%03d.png' % (self.display_mode, self.count)
        #frame = cv.cvtColor(frame, cv.CV_16U)
        cv.imwrite(fn, frame)
        print(fn, 'saved')
        self.count += 1   

    def save_two_images(self, frame):
        "saves two differnet files"
        if len(frame.shape)  < 3:
            print('Image should have 3 chnnels. Try differnet display options')
            return

        fl = '.\\imageL_%s_%03d.png' % (self.display_mode, self.count)
        cv.imwrite(fl, frame[:,:,0])
        fr = '.\\imageR_%s_%03d.png' % (self.display_mode, self.count)
        cv.imwrite(fr, frame[:,:,1])
        print('Saving %s and %s' %(fl,fr))
        self.count += 1          

    def record_video(self, frame):
        # record video to a file is switched on
        if (self.vout is None) and (self.record_on is True):
            fourcc  = cv.VideoWriter_fourcc(*'mp4v')
            k       = 0
            fname   = '.\\video_%s_%03d.mp4' % (self.display_mode,k)
            while os.path.exists(fname):
                k      +=1
                fname   = '.\\video_%s_%03d.mp4' % (self.display_mode,k)

            self.vout     = cv.VideoWriter(fname, fourcc, 20.0, self.frame_size)
            print('Writing video to file %s' %fname)
            self.count = 0

        # write frame
        if (self.vout is not None) and (self.record_on is True):
            ""
            if len(frame.shape) < 3:
                frame = frame[:self.frame_size[1],:self.frame_size[0]]
                frame = cv.cvtColor(frame, cv.COLOR_GRAY2RGB)

            self.vout.write(frame)
            self.count += 1  
            if self.count % 100 == 0:
                print('Writing frame %s' %str(self.count))

        # record on is switched off
        if (self.vout is not None) and (self.record_on is False):
            self.vout.release()
            self.vout = None
            print('Video file created')

    def record_release(self):
        "finish record"         
        if self.vout is not None:
            self.vout.release()
            self.vout = None
            print('Video file created')

    def show_controls(self, frame):
        "show image on opencv window"
        if self.control_mode == 'display':
            frame = cv.putText(frame, 'Display (0-RGB, 1,2,3...9-I1+I2)', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,12), 2)

        elif self.control_mode == 'exposure':
            frame = cv.putText(frame, 'Exposure (1,2,3...9) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)

        elif self.control_mode == 'gain':
            frame = cv.putText(frame, 'Gain (1,2,3...9) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)     

        elif self.control_mode == 'projector':
            frame = cv.putText(frame, 'Projector (0,1) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)                    

        elif self.control_mode == 'disparity':
            frame = cv.putText(frame, 'Disparity Out (0,1) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)  

        elif self.control_mode == 'range':
            frame = cv.putText(frame, 'Bit Range Out (0,1,2...9) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)  
       
        elif self.control_mode == 'focal':
            frame = cv.putText(frame, 'Camera Params (0-BL+F,1-Cam Mtrx+Dist, 2..) ', (10, 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (200,200,200), 2)  
        else:
            pass

        return frame

    def show_measurements(self,frame):
        "show measurements of the noise"
        if not self.use_measure:
            self.img_int_mean = None  # reset when enabled
            return frame
        
        if self.rect is None:         
            h,w          = frame.shape[0]>>1, frame.shape[1]>>1
            h2,w2        = h>>2,w>>2
            self.rect    = [w-h2, h-h2, w+h2, h+h2]        
                
        err_std          = self.measure_noise(frame)
        # show  min and max
        print(f'Frame min {frame.min()} and max {frame.max()}')

        x0, y0, x1, y1   = self.rect
        clr = (0, 0, 0) if frame[y0:y1,x0:x1].mean() > 128 else (240,240,240)
        frame           = cv.rectangle(frame, (x0, y0), (x1, y1), clr, 2)
        frame           = draw_str(frame,(x0,y0-10),str(err_std))
        
        return frame

    def show_image(self, frame):
        "show image on opencv window"
        do_exit     = False
        frame_show  = np.uint8(frame.copy())

        frame_show  = self.show_controls(frame_show)

        cv.imshow('frame (d,e,g,f,p,o,g,m,s,t,r: q - to exit)', frame_show)
        ch = cv.waitKeyEx(1) & 0xff
        if ch == ord('q') or ch == 27:
            do_exit = True 
        elif ch in np.arange(48,58) : # numbers only
            self.set_controls(ch - 48)
        elif ch in np.arange(65,75) : # 2 digit numbers using SHIFT key and keys a,b,c,d,e,f,g
            self.set_controls(ch - 55)            
        elif ch == ord('d'): # depth image
            self.control_mode = 'no controls' if self.control_mode == 'display' else 'display'    
        elif ch == ord('e'): # exposure control
            self.control_mode = 'no controls' if self.control_mode == 'exposure' else 'exposure'     
        elif ch == ord('g'): # exposure control
            self.control_mode = 'no controls' if self.control_mode == 'gain' else 'gain'    
        elif ch == ord('p'): 
            self.control_mode = 'no controls' if self.control_mode == 'projector' else 'projector'  
        elif ch == ord('o'): 
            self.control_mode = 'no controls' if self.control_mode == 'range' else 'range'     
        elif ch == ord('x'): 
            self.control_mode = 'no controls' if self.control_mode == 'disparity' else 'disparity'        
        elif ch == ord('f'): 
            self.control_mode = 'no controls' if self.control_mode == 'focal' else 'focal'                            
        elif ch == ord('m'):   
            self.use_measure = not self.use_measure
            print(f'Noise measurement is {self.use_measure}')
        elif ch == ord('s'):
            self.save_image(frame) 
        elif ch == ord('t'):
            self.save_two_images(frame) 
        elif ch == 2490368: # Left: 2424832 Up: 2490368 Right: 2555904 Down: 2621440
            pass
        elif ch == ord('a'): # enable advanced mode
            self.use_advanced = not self.use_advanced
            self.set_advanced_mode()                                    
        elif ch == ord('r'):
            self.record_on = not self.record_on
            print('Video record %s' %str(self.record_on))
        elif ch != 255:
            print(f'Unrecognized key {ch} - check your language setttings on the keyboard, must be English.')
        return do_exit
          
    def close(self):
        # stop record
        self.record_release()

        # Stop streaming
        self.pipeline.stop()
        #self.depth_sensor.stop()
        #self.depth_sensor.close()
        print('closed')

    def release(self):
        "opencv compatability"
        self.close()

    def test(self):
        while True:
            ret, frame = self.read()
            if ret is False:
                break

            frame = self.show_measurements(frame)
            ret   = self.show_image(frame)
            if ret :
                break  

            # check if record is required
            self.record_video(frame)   

        if ret is False:
            print('Failed to read image')
        else:
            self.close()
        cv.destroyAllWindows()

if __name__ == '__main__':
    cap = RealSense() #frame_size=(640,360))
    cap.test()