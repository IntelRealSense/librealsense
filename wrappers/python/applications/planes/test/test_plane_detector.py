#!/usr/bin/env python

'''
Tester for multi planar plain detector
==================

Using depth image to compute depth planes locally for specific ROI.


Usage:

Environemt : 
    ..\\Envs\\barcode

Install : 



'''

import sys 
import numpy as np
import cv2 as cv
import unittest
from scipy.spatial.transform import Rotation as Rot
import matplotlib.pyplot as plt


# importing common Use modules 
sys.path.append(r'wrappers\python\applications\planes\src')
from plane_detector import PlaneDetector

sys.path.append(r'wrappers\python\applications\utils\src')
from opencv_realsense_camera import RealSense, draw_str
from logger import log


#%% Helpers
def draw_axis(img, rvec, tvec, cam_mtrx, cam_dist, len = 10):
    # unit is mm
    points          = np.float32([[len, 0, 0], [0, len, 0], [0, 0, len], [0, 0, 0]]).reshape(-1, 3)
    axisPoints, _   = cv.projectPoints(points, rvec, tvec, cam_mtrx, cam_dist)
    axisPoints      = axisPoints.squeeze().astype(np.int32)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[0].ravel()), (0,0,255), 3)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[1].ravel()), (0,255,0), 3)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[2].ravel()), (255,0,0), 3)
    return img

def draw_polygon(img, rvec, tvec, cam_mtrx, cam_dist, points3d):
    # unit is mm
    points              = np.float32(points3d).reshape(-1, 3)
    polygon_points, _   = cv.projectPoints(points, rvec, tvec, cam_mtrx, cam_dist)
    polygon_points      = polygon_points.squeeze().astype(np.int32)
    img                 = cv.polylines(img, [polygon_points], True, (0, 200, 200), 1)

    # To fill the polygon, use thickness=-1
    # cv2.fillPoly(img, [pts], color)

    return img

def draw_cube(img, corners, imgpts):
    imgpts = np.int32(imgpts).reshape(-1,2)
    # draw ground floor in green
    img = cv.drawContours(img, [imgpts[:4]],-1,(0,255,0),-3)
    # draw pillars in blue color
    for i,j in zip(range(4),range(4,8)):
        img = cv.line(img, tuple(imgpts[i]), tuple(imgpts[j]),(255),3)

    # draw top layer in red color
    img = cv.drawContours(img, [imgpts[4:]],-1,(0,0,255),3)
    return img

#%% ROI selector from OpenCV
class RectSelector:
    def __init__(self, win, callback):
        self.win = win
        self.callback = callback
        cv.setMouseCallback(win, self.onmouse)
        self.drag_start = None
        self.drag_rect = None
    def onmouse(self, event, x, y, flags, param):
        x, y = np.int16([x, y]) # BUG
        if event == cv.EVENT_LBUTTONDOWN:
            self.drag_start = (x, y)
            return
        if self.drag_start:
            if flags & cv.EVENT_FLAG_LBUTTON:
                xo, yo = self.drag_start
                x0, y0 = np.minimum([xo, yo], [x, y])
                x1, y1 = np.maximum([xo, yo], [x, y])
                self.drag_rect = None
                if x1-x0 > 0 and y1-y0 > 0:
                    self.drag_rect = (x0, y0, x1, y1)
            else:
                rect = self.drag_rect
                self.drag_start = None
                self.drag_rect = None
                if rect:
                    self.callback(rect)
    def draw(self, vis):
        if not self.drag_rect:
            return False
        x0, y0, x1, y1 = self.drag_rect
        cv.rectangle(vis, (x0, y0), (x1, y1), (0, 255, 0), 2)
        return True
    @property
    def dragging(self):
        return self.drag_rect is not None

#%% Data Generator
class DataGen:
    def __init__(self):

        self.frame_size     = (1280,720)
        self.img            = None
        self.rect           = None  # roi  


    def add_noise(self, img_gray, noise_percentage = 0.01):
        "salt and pepper noise"
        if noise_percentage < 0.001:
            return img_gray


        # Get the image size (number of pixels in the image).
        img_size = img_gray.size

        # Set the percentage of pixels that should contain noise
        #noise_percentage = 0.1  # Setting to 10%

        # Determine the size of the noise based on the noise precentage
        noise_size = int(noise_percentage*img_size)

        # Randomly select indices for adding noise.
        random_indices = np.random.choice(img_size, noise_size)

        # Create a copy of the original image that serves as a template for the noised image.
        img_noised = img_gray.copy()

        # Create a noise list with random placements of min and max values of the image pixels.
        #noise = np.random.choice([img_gray.min(), img_gray.max()], noise_size)
        noise = np.random.choice([-10, 10], noise_size)

        # Replace the values of the templated noised image at random indices with the noise, to obtain the final noised image.
        img_noised.flat[random_indices] += noise
        
        log.info('adding image noise')
        return img_noised

    def init_image(self, img_type = 1):
        # create some images for test
        w,h             = self.frame_size
        if img_type == 1: # /
            
            self.img        = np.tile(np.linspace(100, 300, w), (h,1))

        elif img_type == 2: # /|/

            self.img        = np.tile(np.linspace(100, 200, int(w/2)), (h,2))
         
        elif img_type == 3: # |_|

            self.img        = np.tile(np.linspace(100, 200, h).reshape((-1,1)), (1,w)) 
        
        elif img_type == 4: # /\

            self.img        = np.tile(np.hstack((np.linspace(300, 500, w>>1),np.linspace(500, 300, w>>1))), (h,1))        

        elif img_type == 5: # dome

            x,y             = np.meshgrid(np.arange(w),np.arange(h))   
            self.img        = (np.abs(x - w/2) + np.abs(y - h/2))/10 + 200 # less slope

        elif img_type == 6: # sphere

            x,y             = np.meshgrid(np.arange(w),np.arange(h))   
            self.img        = np.sqrt((x - w/2)**2 + (y - h/2)**2)/10 + 200 # less slope   

        elif img_type == 7: # stair

            x,y             = np.meshgrid(np.arange(w),np.arange(h))   
            self.img        = (np.sign(x - w/2) + np.sign(y - h/2))*5 + 200 # less slope     


        elif img_type == 8: # corner

            x,y             = np.meshgrid(np.arange(w),np.arange(h))   
            self.img        = np.ones((h,w))*250
            img_bool        = np.logical_and((x - w/2) < 0, (y - h/2) < 0)
            self.img[img_bool] = 230 # quarter                            

        elif img_type == 10: # flat

            self.img        = np.ones((h,w))*500             

        elif img_type == 11:
            "chess board"
            fname           = r"C:\Users\udubin\Documents\Code\opencv-4x\samples\data\left04.jpg"
            self.img        = cv.imread(fname)

        elif img_type == 12:
            self.img = cv.imread('image_scl_001.png', cv.IMREAD_GRAYSCALE)
            #self.img = cv.resize(self.img , dsize = self.frame_size) 
            
        elif img_type == 13:
            self.img = cv.imread(r"wrappers\python\applications\planes\data\image_ddd_000.png", cv.IMREAD_GRAYSCALE)
            #self.img = cv.resize(self.img , dsize = self.frame_size) 

        elif img_type == 21:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_scl_000.png", cv.IMREAD_GRAYSCALE)  
            #self.img = cv.resize(self.img , dsize = self.frame_size)                                     
            
        #self.img        = np.uint8(self.img) 

        self.img = self.add_noise(self.img, 0)
        self.frame_size = self.img.shape[:2]      
        return self.img
      
    def init_roi(self, test_type = 1):
        "load the test case"
        roi = [0,0,self.frame_size[0],self.frame_size[1]]
        if test_type == 1:
            roi = [310,230,330,250] # xlu, ylu, xrb, yrb
        elif test_type == 2:
            roi = [300,220,340,260] # xlu, ylu, xrb, yrb
        elif test_type == 3:
            roi = [280,200,360,280] # xlu, ylu, xrb, yrb            
        elif test_type == 4:
            roi = [220,140,420,340] # xlu, ylu, xrb, yrb      
        elif test_type == 4:
            roi = [200,120,440,360] # xlu, ylu, xrb, yrb            
        return roi    
    
    def test_image(self):
        "test single image depth"
        img  = self.init_image(1)
        roi  = self.init_roi(1)      

#%% Adds display functionality to the PlaneDetector
class PlaneDetectorDisplay(PlaneDetector):
    def __init__(self, detect_type='p'):
        super().__init__(detect_type)
        self.detect_type    = detect_type

        self.frame_size     = (1280,720)
        self.img            = None

    def show_image_with_axis(self, img, poses = []):
        "draw results : axis on the image. poses are list of 6D vectors"
        axis_number = len(poses)
        if axis_number < 1:
            log.error('No poses found')
            
        # deal with black and white
        img_show = np.uint8(img) #.copy()
        if len(img.shape) < 3:
            img_show = cv.applyColorMap(img_show, cv.COLORMAP_JET)
         
        for k in range(axis_number):
            
            rvec    = poses[k][3:] # orientation in degrees
            tvec    = poses[k][:3] #np.array(, dtype = np.float32).reshape(rvec.shape) # center of the patch
            img_show= draw_axis(img_show, rvec, tvec, self.cam_matrix, self.cam_distort, len = 10)

        cv.imshow('Image & Axis', img_show)
        log.info('show done')
        ch = cv.waitKey()

    def show_image_with_rois(self, img, roi_params_ret = []):
        "draw results by projecting ROIs on image"

        axis_number = len(roi_params_ret)
        if axis_number < 1:
            print('No poses found')
            
        # deal with black and white
        img_show = np.uint8(img) #.copy()
        if len(img.shape) < 3:
            img_show = cv.applyColorMap(img_show, cv.COLORMAP_JET)
         
        for roi_p in roi_params_ret:

            pose    = self.convert_roi_params_to_pose(roi_p)            
            
            avec    = pose[3:6] # orientation in degrees
            levl    = pose[6]   # level
            #R       = eulerAnglesToRotationMatrix(avec)
            R       = Rot.from_euler('zyx',avec, degrees = True).as_matrix()
            rvec, _ = cv.Rodrigues(R)
            tvec    = np.array(pose[:3], dtype = np.float32).reshape(rvec.shape) # center of the patch
            img_show= draw_axis(img_show, rvec, tvec, self.cam_matrix, self.cam_distort, len = levl)

        cv.imshow('Image & Axis', img_show)
        log.info('show done')
        ch = cv.waitKey()

    def show_points_3d_with_normal(self, img3d, pose = None):
        "display in 3D"
        fig = plt.figure()
        ax  = fig.add_subplot(projection='3d')

        #xs,ys,zs       = img3d[:,:,0].reshape((-1,1)), img3d[:,:,1].reshape((-1,1)), img3d[:,:,2].reshape((-1,1))
        
        xs,ys,zs       = img3d[:,0].reshape((-1,1)), img3d[:,1].reshape((-1,1)), img3d[:,2].reshape((-1,1))
        ax.scatter(xs, ys, zs, marker='.')
        
        if pose is not None:
            pose       = pose.flatten()
            vnorm      = pose[3:6].flatten()*10
            xa, ya, za = [pose[0], pose[0]+vnorm[0]], [pose[1], pose[1]+vnorm[1]], [pose[2], pose[2]+vnorm[2]]
            ax.plot(xa, ya, za, 'r', label='Normal')


        ax.set_xlabel('X [mm]')
        ax.set_ylabel('Y [mm]')
        ax.set_zlabel('Z [mm]')
        ax.set_aspect('equal', 'box')
        plt.show()

    def show_rois_3d_with_normals(self, roi_params_ret = [], roi_init = None):
        "display in 3D each ROI region with split"
        
        if len(roi_params_ret) < 1:
            log.info('roi_params_ret is empty')
            return

        # extract the initial ROI - to make the show more compact
        roi_init       = [0,0,self.frame_size[1], self.frame_size[0]] if roi_init is None else roi_init
        x0,y0,x1,y1    = roi_init

        if self.img_xyz is None:
            log.info('Need init')
            return      

        img3d          = self.img_xyz[y0:y1,x0:x1,:] 
        xs,ys,zs       = img3d[:,:,0].reshape((-1,1)), img3d[:,:,1].reshape((-1,1)), img3d[:,:,2].reshape((-1,1))

        fig = plt.figure()
        ax = fig.add_subplot(projection='3d')
        ax.scatter(xs, ys, zs, marker='.')
        
        for roi_p in roi_params_ret:
            pose       = self.convert_roi_params_to_pose(roi_p)
            pose       = pose.flatten()
            # R          = Rot.from_euler('zyx',pose[3:6],degrees=True).as_matrix()
            # vnorm      = R[:,2]*pose[6]
            vnorm      = pose[3:6]*pose[6]
            #log.info(str(vnorm))
            xa, ya, za = [pose[0], pose[0]+vnorm[0]], [pose[1], pose[1]+vnorm[1]], [pose[2], pose[2]+vnorm[2]]
            ax.plot(xa, ya, za, 'r', label='Normal')


        ax.set_xlabel('X [mm]')
        ax.set_ylabel('Y [mm]')
        ax.set_zlabel('Z [mm]')
        ax.set_aspect('equal', 'box')
        plt.show() #block=False)        

    def show_axis(self, vis):
        "draw axis after plane estimation"
        if self.plane_params is None:
            return vis
        
        #rvec = self.plane_params/np.sum(self.plane_params**2) # normalize
        rvec = self.convert_plane_params(self.plane_params)
        #rvec = self.convert_plane_to_rvec(self.plane_params)
        
        tvec = self.plane_center
        vis  = draw_axis(vis, rvec, tvec, self.cam_matrix, self.cam_distort, len = 50)
        return vis
    
    def show_text(self, vis):
        "draw text plane estimation"
        err_mean, err_std = self.img_mean, self.img_std
        if err_mean is None:
            return vis
        
        if self.rect is None:
            return vis
        
        x0, y0, x1, y1 = self.rect
        txt = f'{self.detect_type}:{err_mean:.2f}:{err_std:.3f}'
        if self.detect_type == 'F':
            txt = f'{self.detect_type}:{self.img_fill:.2f} %'
        vis = draw_str(vis,(x0,y0-10),txt)

        return vis 

    def show_rect_and_text(self, vis):
        "draw axis after plane estimation"
        err_mean, err_std = self.img_mean, self.img_std
        if err_mean is None:
            return vis
        
        if self.rect is None:
            return vis
        
        x0, y0, x1, y1 = self.rect
        clr = (0, 0, 0) if vis[y0:y1,x0:x1].mean() > 128 else (240,240,240)
        vis = cv.rectangle(vis, (x0, y0), (x1, y1), clr, 2)
        txt = f'{self.detect_type}:{err_mean:.2f}-{err_std:.3f}'
        if self.detect_type == 'F':
            txt = f'{self.detect_type}:{self.img_fill:.2f} %'
        vis = draw_str(vis,(x0,y0-10),txt)

        return vis 

    def show_rect_and_axis_projected(self, vis):
        "projects rectangle on the plane"
        if self.rect is None:
            return vis
        if self.plane_params is None:
            return vis
        
        rvec = self.convert_plane_params(self.plane_params)
        tvec = self.plane_center

        vis  = draw_axis(vis, rvec, tvec, self.cam_matrix, self.cam_distort, len = 50)        
        vis  = draw_polygon(vis, rvec, tvec, self.cam_matrix, self.cam_distort, self.rect_3d)
    
        return vis 

    def show_mask(self, img):
        "draw image mask"

        # deal with black and white
        img_show = img #np.uint8(img) #.copy()
        if len(img.shape) < 3:
            img_show = cv.applyColorMap(img_show, cv.COLORMAP_JET)

        if not np.all(self.img_mask.shape[:2] == img_show.shape[:2]):
            log.error('mask and image size are not equal')
            return img_show
        
        img_show[self.img_mask == 1] = self.color_mask
        return img_show

    def show_scene(self, vis):
        "draw ROI and Info"

        #vis = self.show_rect_and_text(vis)
        #vis = self.show_axis(vis)

        vis = self.show_rect_and_axis_projected(vis)
        vis = self.show_text(vis)

        vis = self.show_mask(vis)

        return vis  
        

# ----------------------
#%% Tests
class TestPlaneDetector(unittest.TestCase):

    def test_image_show(self):
        "checking image show"
        d       = DataGen()
        img     = d.init_image(1)
        p       = PlaneDetectorDisplay()
        poses   = [[0,0,100,0,0,45,10]]
        p.show_image_with_axis(img,poses)
        self.assertFalse(d.img is None)    

    def test_init_img3d(self):
        "XYZ point cloud structure init"
        d       = DataGen()
        img     = d.init_image(1)
        p       = PlaneDetectorDisplay()
        isOk    = p.init_image(img)
        img3d   = p.init_img3d()
        self.assertFalse(img3d is None)    

    def test_compute_img3d(self):
        "XYZ point cloud structure init and compute"
        d       = DataGen()
        img     = d.init_image(1)        
        p       = PlaneDetectorDisplay()
        img3d   = p.init_img3d(img)
        imgXYZ  = p.compute_img3d(img)
        self.assertFalse(imgXYZ is None)     

    def test_show_img3d(self):
        "XYZ point cloud structure init and compute"
        d       = DataGen()
        img     = d.init_image(1)        
        p       = PlaneDetectorDisplay()
        img3d   = p.init_img3d(img)
        imgXYZ  = p.compute_img3d(img)
        roi     = p.init_roi(1)
        x0,y0,x1,y1 = roi
        roiXYZ    = imgXYZ[y0:y1,x0:x1,:]
        p.show_points_3d_with_normal(roiXYZ)
        self.assertFalse(imgXYZ is None)  
                     
    def test_fit_plane_svd(self):
        "computes normal to the ROI"
        d           = DataGen()
        img         = d.init_image(5)        
        p           = PlaneDetectorDisplay()
        roi         = p.init_roi(4)
        img_roi     = p.preprocess(img)
        roim,rois   = p.fit_plane_svd(img_roi)
        pose        = p.convert_plane_params_to_pose()
        p.show_image_with_axis(img, pose)
        p.show_points_3d_with_normal(p.matrix_xyz, pose)
        self.assertTrue(pose[0][2] > 0.01)         

    def test_fit_plane_depth_image(self):
        "computes normal to the ROI"
        d           = DataGen()
        img         = d.init_image(13)        
        p           = PlaneDetectorDisplay()
        roi         = p.init_roi(4)
        img_roi     = p.preprocess(img)
        roim,rois   = p.fit_plane_svd(img_roi)
        pose        = p.convert_plane_params_to_pose()
        p.show_image_with_axis(img, pose)
        p.show_points_3d_with_normal(p.matrix_xyz, pose)
        self.assertTrue(pose[0][2] > 0.01)  

    def test_fit_plane_with_outliers(self):
        "computes normal to the ROI"
        d           = DataGen()
        img         = d.init_image(13)        
        p           = PlaneDetectorDisplay()
        roi         = p.init_roi(4)
        img_roi     = p.preprocess(img)
        roim,rois   = p.fit_plane_with_outliers(img_roi)
        pose        = p.convert_plane_params_to_pose()
        p.show_image_with_axis(img, pose)
        p.show_points_3d_with_normal(p.matrix_xyz, pose)
        self.assertTrue(pose[0][2] > 0.01)  

    def test_fit_plane_ransac(self):
        "computes with ransac"
        d           = DataGen()
        img         = d.init_image(6)        
        p           = PlaneDetectorDisplay()
        roi         = p.init_roi(4)
        img_roi     = p.preprocess(img)
        roim,rois   = p.fit_plane_ransac(img_roi)
        pose        = p.convert_plane_params_to_pose()
        p.show_image_with_axis(img, pose)
        p.show_points_3d_with_normal(p.matrix_xyz, pose)
        self.assertTrue(pose[0][2] > 0.01)  

    def test_split_roi(self):
        "computes ROIS and splits if needed"
        p       = PlaneDetector()
        p.MIN_STD_ERROR = 0.1
        img     = p.init_image(13)
        roi     = p.init_roi(4)
        img3d   = p.init_img3d(img)
        imgXYZ  = p.compute_img3d(img)
        roi_list= p.fit_and_split_roi_recursively(roi)
        p.show_rois_3d_with_normals(roi_list, roi)
        p.show_image_with_rois(p.img, roi_list)

        for roi_s in roi_list:
            self.assertFalse(roi_s['error'] > 0.01)                               

#%% Run Test
def RunTest():
    #unittest.main()
    suite = unittest.TestSuite()
    #suite.addTest(TestPlaneDetector("test_image_show")) # ok
    #suite.addTest(TestPlaneDetector("test_init_img3d"))  # ok
    #suite.addTest(TestPlaneDetector("test_compute_img3d")) # ok
    #suite.addTest(TestPlaneDetector("test_show_img3d")) # ok
    
    #suite.addTest(TestPlaneDetector("test_fit_plane_svd")) # ok
    #suite.addTest(TestPlaneDetector("test_fit_plane_depth_image")) #
    #suite.addTest(TestPlaneDetector("test_fit_plane_with_outliers")) 
    suite.addTest(TestPlaneDetector("test_fit_plane_ransac"))    
    
    #suite.addTest(TestPlaneDetector("test_split_roi")) 
    

   
    runner = unittest.TextTestRunner()
    runner.run(suite)    

# ----------------------
#%% App
class PlaneApp:
    def __init__(self):
        self.cap            = RealSense() #
        self.cap.set_display_mode('d16')
        self.cap.set_exposure(5000)
        self.frame          = None
        self.rect           = None
        self.paused         = False
        self.trackers       = []

        self.show_dict      = {} # hist show

        self.detect_type    = 'P'
        self.show_type      = 'depth' # left, depth
        self.win_name       = 'Plane Detector (q-quit, c-clear, a,r,p,o,g,s)'

        cv.namedWindow(self.win_name )
        self.rect_sel       = RectSelector(self.win_name , self.on_rect)

    def on_rect(self, rect):
        "remember ROI defined by user"
        #self.define_roi(self.frame, rect)
        tracker             = PlaneDetectorDisplay() #estimator_type=self.estim_type, estimator_id=estim_ind)
        tracker.rect        = rect
        tracker.detect_type = self.detect_type
        self.trackers.append(tracker)        
        log.info(f'Adding plane estimator at  : {rect}') 

    def process_image(self, img_depth):
        "makes measurements"
        for tracker in self.trackers:
            tracker.find_planes(img_depth) 

    def show_scene(self, frame):
        "draw ROI and Info"
        if self.show_type == 'left':
            vis     = frame[:,:,0].astype(np.uint8)
        else:
            vis     = cv.convertScaleAbs(frame[:,:,2], alpha=0.1).astype(np.uint8)

        vis     = cv.cvtColor(vis, cv.COLOR_GRAY2BGR)
        self.rect_sel.draw(vis)

        for tracker in self.trackers:
            vis = tracker.show_scene(vis) 

        return vis 
    
    def show_histogram(self, img):
        "show roi histgram"
        if self.rect is None:
            #print('define ROI')
            return 0
        
        x0, y0, x1, y1 = self.rect
        img_roi = img[y0:y1,x0:x1].astype(np.float32)
        # Compute histogram
        hist, bins = np.histogram(img_roi.flatten(), bins=1024, range=[0, 2**15])

        if not 'fig' in self.show_dict : #len(self.show_dict) < 1:
            fig, ax = plt.subplots()
            fig.set_size_inches([24, 16])
            ax.set_title('Histogram (Depth)')
            ax.set_xlabel('Bin')
            ax.set_ylabel('Frequency')
            lineGray, = ax.plot(bins[:-1], hist, c='k', lw=3)
            ax.set_xlim(bins[0], bins[-1])
            ax.set_ylim(0, max(hist)+10)
            plt.ion()
            #plt.show()

            self.show_dict = {'fig':fig, 'ax':ax, 'line':lineGray}
        else:
            self.show_dict['line'].set_ydata(hist)
        
        self.show_dict['fig'].canvas.draw()
        return

    def run(self):
        while True:
            playing = not self.paused and not self.rect_sel.dragging
            if playing or self.frame is None:
                ret, frame = self.cap.read()
                if not ret:
                    break
                self.frame = frame.copy()

            
            #self.statistics(frame)
            img_depth = self.frame[:,:,2]    
            self.process_image(img_depth)

            #self.show_histogram(frame)

            vis     = self.show_scene(frame)
            cv.imshow(self.win_name , vis)
            ch = cv.waitKey(1)
            if ch == ord(' '):
                self.paused = not self.paused
            elif ch == ord('a'):
                self.detect_type = 'A' 
                log.info(f'Detect type : {self.detect_type}')
            elif ch == ord('r'):
                self.detect_type = 'R'  
                log.info(f'Detect type : {self.detect_type}')
            elif ch == ord('p'):
                self.detect_type = 'P'  
                log.info(f'Detect type : {self.detect_type}')
            elif ch == ord('o'):
                self.detect_type = 'O'  
                log.info(f'Detect type : {self.detect_type}') 
            elif ch == ord('g'):
                self.detect_type = 'G'    
                log.info(f'Detect type : {self.detect_type}')                
            elif ch == ord('s'):
                self.show_type = 'left' if self.show_type == 'depth' else 'depth'      
                log.info(f'Show type : {self.show_type}')                                    
            if ch == ord('c'):
                if len(self.trackers) > 0:
                    t = self.trackers.pop()
            if ch == 27 or ch == ord('q'):
                break              


if __name__ == '__main__':
    #print(__doc__)

    #RunTest()
    PlaneApp().run()



