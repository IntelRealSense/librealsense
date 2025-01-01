#!/usr/bin/env python

'''
Utility functions for multi planar plain detector
==================



Usage:

Environemt : 
    C:\\Users\\udubin\\Documents\\Envs\\barcode

Install : 


'''

import numpy as np
import cv2 as cv
import unittest
from scipy.spatial.transform import Rotation as Rot
#from scipy import interpolate 

 # importing common Use modules 
import sys 
sys.path.append(r'..\Utils\src')
from opencv_realsense_camera import RealSense

import logging as plog
plog.basicConfig(stream=sys.stdout, level=plog.DEBUG, format='[%(asctime)s.%(msecs)03d] {%(filename)s:%(lineno)d} %(levelname)s - %(message)s',  datefmt="%M:%S")
plog.getLogger('matplotlib.font_manager').disabled = True
plog.getLogger('matplotlib').setLevel(plog.WARNING)
plog.getLogger('PIL').setLevel(plog.WARNING)

import matplotlib.pyplot as plt

#%% Helpers
def draw_axis(img, rvec, tvec, cam_mtrx, cam_dist, len = 10):
    # unit is mm
    points          = np.float32([[len, 0, 0], [0, len, 0], [0, 0, len], [0, 0, 0]]).reshape(-1, 3)
    axisPoints, _   = cv.projectPoints(points, rvec, tvec, cam_mtrx, cam_dist)
    axisPoints      = axisPoints.squeeze().astype(np.int32)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[0].ravel()), (255,0,0), 3)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[1].ravel()), (0,255,0), 3)
    img = cv.line(img, tuple(axisPoints[3].ravel()), tuple(axisPoints[2].ravel()), (0,0,255), 3)
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


#%% Main
class PointGenerator:
    "class to create images and points"
    def __init__(self):

        self.frame_size = (640,480)
        self.img        = None

        self.cam_matrix = np.array([[1000,0,self.frame_size[0]/2],[0,1000,self.frame_size[1]/2],[0,0,1]], dtype = np.float32)
        self.cam_distort= np.array([0,0,0,0,0],dtype = np.float32)        

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
        
        self.tprint('adding image noise')
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
            fname = r"C:\Users\udubin\Documents\Code\opencv-4x\samples\data\left04.jpg"
            self.img        = cv.imread(fname)

        elif img_type == 12:
            self.img = cv.imread('image_scl_001.png', cv.IMREAD_GRAYSCALE)
            self.img = cv.resize(self.img , dsize = self.frame_size) 
            
        elif img_type == 13:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_ddd_000.png", cv.IMREAD_GRAYSCALE)
            self.img = cv.resize(self.img , dsize = self.frame_size) 

        elif img_type == 14:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_ddd_001.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)     

        elif img_type == 15:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_ddd_002.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)     

        elif img_type == 16:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_ddd_003.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)   

        elif img_type == 17:
            self.img = cv.imread(r"C:\Data\Depth\Plane\floor_view_default_Depth_Depth.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)          

        elif img_type == 18:
            self.img = cv.imread(r"C:\Data\Depth\Plane\floor_view_default_corner2_Depth_Depth.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)                   

        elif img_type == 21:
            self.img = cv.imread(r"C:\Data\Depth\Plane\image_scl_000.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)       

        elif img_type == 31:
            self.img = cv.imread(r"C:\Users\udubin\Documents\Projects\Planes\data\image_ggd_053.png", cv.IMREAD_GRAYSCALE)  
            self.img = cv.resize(self.img , dsize = self.frame_size)                                           
            
        #self.img        = np.uint8(self.img) 

        self.img = self.add_noise(self.img, 0)
        self.frame_size = self.img.shape[:2]      
        return self.img
      
    def init_roi(self, test_type = 1):
        "load the test case"
        roi = [0,0,self.frame_size[1],self.frame_size[0]]
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

    def init_img3d(self, img = None):
        "initializes xyz coordinates for each point"
        img     = self.img if img is None else img
        h,w     = img.shape[:2]
        x       = np.arange(w)
        y       = np.arange(h)
        x,y     = np.meshgrid(x,y)
        #fx      = self.cam_matrix[0,0]
        #fy      = self.cam_matrix[1,1]
        
        xy      = np.hstack((x.reshape(-1,1),y.reshape(-1,1)))
        xy      = np.expand_dims(xy, axis=1).astype(np.float32)
        xy_undistorted = cv.undistortPoints(xy, self.cam_matrix, self.cam_distort)

        u       = xy_undistorted[:,0,0].reshape((h,w))
        v       = xy_undistorted[:,0,1].reshape((h,w))
        z3d     = img.astype(np.float32)
        x3d     = z3d.copy()
        y3d     = z3d.copy()


        # filter bad z values
        ii          = z3d > 5
        x3d[ii]     = x3d[ii]*z3d[ii]
        y3d[ii]     = y3d[ii]*z3d[ii]
        z3d[ii]     = z3d[ii]

        # z3d[z3d < 5] = 5
        # x3d          = x3d*z3d
        # y3d          = y3d*z3d

        #self.img3d = np.stack((u/fx,v/fy,z3d), axis = 2)
        self.img3d      = np.stack((u,v,z3d), axis = 2)
        self.imgMask    = np.zeros((h,w))
        self.frame_size = img.shape[:2]
        return self.img3d
    
    def compute_img3d(self, img = None):
        "compute xyz coordinates for each point using prvious init"
        img         = self.img if img is None else img
        xyz         = self.img3d
        if xyz is None:
            xyz = self.init_img3d(img)

        if np.any(img.shape[:2] != xyz.shape[:2]):
            print('Image dimension change')
            return 

        imgXYZ      = self.img3d.copy()

        z3d         = img.astype(np.float32)
        x3d         = self.img3d[:,:,0].copy()  # u/f
        y3d         = self.img3d[:,:,1].copy()  # v/f

        # filter bad z values
        #ii          = np.logical_and(z3d > 1e-6 , np.isfinite(z3d))
        ii          = z3d > 5
        x3d[ii]     = x3d[ii]*z3d[ii]
        y3d[ii]     = y3d[ii]*z3d[ii]
        z3d[ii]     = z3d[ii]

        # z3d[z3d < 5] = 5
        # x3d          = x3d*z3d
        # y3d          = y3d*z3d
     

        # x,y,z coordinates in 3D
        imgXYZ[:,:,0] = x3d
        imgXYZ[:,:,1] = y3d
        imgXYZ[:,:,2] = z3d

        self.imgXYZ = imgXYZ
        return imgXYZ   

    def convert_roi_to_points(self, roi, step_size = 0):
        "converting roi to pts in XYZ - Nx3 array"
        x0,y0,x1,y1 = roi

        # reduce size of the grid for speed
        if step_size < 1:
            roi_area    = (x1-x0)*(y1-y0)
            step_size   = np.maximum(1,int(np.sqrt(roi_area)/10))

        roi3d       = self.imgXYZ[y0:y1:step_size,x0:x1:step_size,:]   
        if roi3d .shape[0] < 5:
            self.tprint('Too small region: %d' %step_size)
            step_size   = 1
            roi3d       = self.imgXYZ[y0:y1:step_size,x0:x1:step_size,:]   

        x,y,z       = roi3d[:,:,0].reshape((-1,1)), roi3d[:,:,1].reshape((-1,1)), roi3d[:,:,2].reshape((-1,1)) 
        xyz_matrix = np.hstack((x,y,z))   

        return xyz_matrix      
    
    def init_point_cloud(self, point_type = 1, roi_type = 1):
        "init entire point cloud"
        img     = self.init_image(point_type)
        img3d   = self.init_img3d(img)
        imgXYZ  = self.compute_img3d(img)
        roi     = self.init_roi(roi_type)
        points  = self.convert_roi_to_points(roi, step_size = 1)
        points  = points.astype(np.float64)
        return points
    
    def init_point_cloud_from_image(self, img, roi_type = 0):
        "init entire point cloud from depth image"
        img3d   = self.init_img3d(img)
        imgXYZ  = self.compute_img3d(img)
        roi     = self.init_roi(roi_type)
        points  = self.convert_roi_to_points(roi, step_size = 1)
        points  = points.astype(np.float64)
        return points    

    def show_image_with_axis(self, img, poses = []):
        "draw results"
        axis_number = len(poses)
        if axis_number < 1:
            print('No poses found')
            
        # deal with black and white
        img_show = np.uint8(img) #.copy()
        if len(img.shape) < 3:
            img_show = cv.applyColorMap(img_show, cv.COLORMAP_JET)
         
        for k in range(axis_number):
            
            avec    = poses[k][3:6] # orientation in degrees
            levl    = poses[k][6]   # level
            #R       = eulerAnglesToRotationMatrix(avec)
            R       = Rot.from_euler('xyz',avec, degrees = True).as_matrix()
            rvec, _ = cv.Rodrigues(R)
            tvec    = np.array(poses[k][:3], dtype = np.float32).reshape(rvec.shape) # center of the patch
            img_show= draw_axis(img_show, rvec, tvec, self.cam_matrix, self.cam_distort, len = levl)

        cv.imshow('Image & Axis', img_show)
        self.tprint('show done')
        ch = cv.waitKey()

    def show_points_3d_with_normal(self, img3d, pose = None):
        "display in 3D"
        fig = plt.figure()
        ax = fig.add_subplot(projection='3d')

        xs,ys,zs       = img3d[:,:,0].reshape((-1,1)), img3d[:,:,1].reshape((-1,1)), img3d[:,:,2].reshape((-1,1))
        ax.scatter(xs, ys, zs, marker='.')
        
        if pose is not None:
            pose       = pose.flatten()
            # R          = Rot.from_euler('zyx',pose[3:6],degrees=True).as_matrix()
            # vnorm      = R[:,2]*pose[6]
            vnorm      = pose[3:6].flatten()*pose[6]
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
            self.tprint('roi_params_ret is empty')
            return

        # extract the initial ROI - to make the show more compact
        roi_init       = [0,0,self.frame_size[1], self.frame_size[0]] if roi_init is None else roi_init
        x0,y0,x1,y1    = roi_init

        if self.imgXYZ is None:
            self.tprint('Need init')
            return      

        img3d          = self.imgXYZ[y0:y1,x0:x1,:] 
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
            #self.tprint(str(vnorm))
            xa, ya, za = [pose[0], pose[0]+vnorm[0]], [pose[1], pose[1]+vnorm[1]], [pose[2], pose[2]+vnorm[2]]
            ax.plot(xa, ya, za, 'r', label='Normal')


        ax.set_xlabel('X [mm]')
        ax.set_ylabel('Y [mm]')
        ax.set_zlabel('Z [mm]')
        ax.set_aspect('equal', 'box')
        plt.show() #block=False)           

    def tprint(self, txt = '', level = 'I'):
        if level == "I":
            plog.info(txt)
        elif level == "W":
            plog.warning(txt)
        elif level == "E":
            plog.error(txt)
        else:
            plog.info(txt)
   


# ----------------------
#%% Tests
class TestPointGenerator(unittest.TestCase):

    def test_image_show(self):
        p = PointGenerator()
        p.init_image(1)
        poses = [[0,0,100,0,0,45,10]]
        p.show_image_with_axis(p.img,poses)
        self.assertFalse(p.img is None)

    def test_init_img3d(self):
        "XYZ point cloud structure init"
        p = PointGenerator()
        p.init_image(1)
        img3d = p.init_img3d()
        self.assertFalse(img3d is None)    

    def test_compute_img3d(self):
        "XYZ point cloud structure init and compute"
        p       = PointGenerator()
        img     = p.init_image(1)
        img3d   = p.init_img3d(img)
        imgXYZ  = p.compute_img3d(img)
        self.assertFalse(imgXYZ is None)     

    def test_show_img3d(self):
        "XYZ point cloud structure init and compute"
        p       = PointGenerator()
        img     = p.init_image(1)
        img3d   = p.init_img3d(img)
        imgXYZ  = p.compute_img3d(img)
        roi     = p.init_roi(1)
        x0,y0,x1,y1 = roi
        roiXYZ    = imgXYZ[y0:y1,x0:x1,:]
        p.show_points_3d_with_normal(roiXYZ)
        self.assertFalse(imgXYZ is None)                      



if __name__ == '__main__':
    #print(__doc__)

    #unittest.main()
    suite = unittest.TestSuite()
    suite.addTest(TestPointGenerator("test_image_show"))
    suite.addTest(TestPointGenerator("test_init_img3d")) # ok
    suite.addTest(TestPointGenerator("test_compute_img3d")) # ok
    suite.addTest(TestPointGenerator("test_show_img3d")) # 
   
    runner = unittest.TextTestRunner()
    runner.run(suite)

