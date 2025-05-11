#!/usr/bin/env python

'''
Multi planar plain detector
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


 # importing common Use modules 
sys.path.append(r'wrappers\python\applications\utils\src')
from logger import log


#%% Main
class PlaneDetector:
    def __init__(self, detect_type = 'p'):

        self.detect_type    = detect_type   # plane

        self.frame_size     = (1280,720)
        self.img            = None
        self.cam_matrix     = np.array([[1000,0,self.frame_size[0]/2],[0,1000,self.frame_size[1]/2],[0,0,1]], dtype = np.float32)
        self.cam_distort    = np.array([0,0,0,0,0],dtype = np.float32)

        self.img3d          = None # contains x,y and depth plains
        self.img_xyz        = None  # comntains X,Y,Z information after depth image to XYZ transform
        self.img_mask       = None  # which pixels belongs to which cluster
        self.rect           = None  # roi  


        # detector type     
        self.matrix_inv     = None     # holds inverse params of the 
        self.matrix_dir     = None     # direct u,v,1
        self.plane_params   = None     # rvec not normalized
        self.plane_center   = None     # tvec
        #self.corner_ind     = [0, 10, 40, 50]  # corner of the rectnagle for the projection
        self.rect_3d        = None    # roi but projected on 3D 

        # params
        self.MIN_SPLIT_SIZE  = 32
        self.MIN_STD_ERROR   = 0.01

        # help variable
        self.ang_vec     = np.zeros((3,1))  # help variable

    def init_image(self, img = None):
        "load image"
        if img is None:
            log.info('No image provided')
            return False
        
        self.img            = img
        h,w                 = img.shape[:2]
        self.frame_size     = (w,h)
        return True

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

    def preprocess(self, img):
        "image preprocessing - extracts roi and converts from uint8 to float using log function"
        if self.rect is None: # use entire image
            self.rect = self.init_roi(4)
            
        x0, y0, x1, y1  = self.rect
        if len(img.shape) > 2:
            img_roi        = img[y0:y1,x0:x1,2].astype(np.float32)
        else:
            img_roi        = img[y0:y1,x0:x1].astype(np.float32)
        return img_roi         

    def init_img3d(self, img = None):
        "initializes xyz coordinates for each point"
        img     = self.img if img is None else img
        h,w     = img.shape[:2]
        x       = np.arange(w)
        y       = np.arange(h)
        x,y     = np.meshgrid(x,y)
        fx      = self.cam_matrix[0,0]
        fy      = self.cam_matrix[1,1]
        
        xy      = np.hstack((x.reshape(-1,1),y.reshape(-1,1)))
        xy      = np.expand_dims(xy, axis=1).astype(np.float32)
        xy_undistorted = cv.undistortPoints(xy, self.cam_matrix, self.cam_distort)

        u       = xy_undistorted[:,0,0].reshape((h,w))
        v       = xy_undistorted[:,0,1].reshape((h,w))
        z3d     = img.astype(np.float32)
        x3d     = z3d.copy()
        y3d     = z3d.copy()

        #ii        = np.logical_and(z3d> 1e-6 , np.isfinite(z3d))
        ii        = z3d > 5
        x3d[ii]   = u[ii]*z3d[ii] #/fx
        y3d[ii]   = v[ii]*z3d[ii] #/fy
        z3d[ii]   = z3d[ii]

        #self.img3d = np.stack((u/fx,v/fy,z3d), axis = 2)
        self.img3d      = np.stack((u,v,z3d), axis = 2)
        self.img_mask    = np.zeros((h,w))
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
        ii          = z3d > 15
        x3d[ii]     = x3d[ii]*z3d[ii]
        y3d[ii]     = y3d[ii]*z3d[ii]
        z3d[ii]     = z3d[ii]

        # x,y,z coordinates in 3D
        imgXYZ[:,:,0] = x3d
        imgXYZ[:,:,1] = y3d
        imgXYZ[:,:,2] = z3d

        self.img_xyz = imgXYZ
        return imgXYZ

    def check_error(self, xyz1_mtrx, vnorm):
        "checking the error norm"
        err         = np.dot(xyz1_mtrx, vnorm)
        err_std     = err.std()
        return err_std
    
    def convert_roi_to_points(self, img_roi, point_num = 30, step_size = 0):
        "converting roi to pts in XYZ - Nx3 array. point_num - is the target point number"
        # x1,y1       = self.img_xyz.shape[:2]
        # roi_area    = x1*y1

        # # reduce size of the grid for speed
        # if step_size < 1 and roi_area > 100:
        #     step_size   = np.maximum(1,int(np.sqrt(roi_area)/10))

          
        # #roi3d       = self.img_xyz[y0:y1:step_size,x0:x1:step_size,:]   
        # roi3d       = self.img_xyz[::step_size,::step_size,:]           
        # x,y,z       = roi3d[:,:,0].reshape((-1,1)), roi3d[:,:,1].reshape((-1,1)), roi3d[:,:,2].reshape((-1,1)) 
        # xyz_matrix  = np.hstack((x,y,z)) 
        # 
        
        # init params of the inverse
        if self.matrix_inv is None:
            self.fit_plane_init()  

        n,m                 = img_roi.shape[:2]
        img_roi             = img_roi.reshape((-1,1))
        valid_bool          = img_roi > 0
        valid_bool          = valid_bool.flatten()
        #log.info(f'Timing : 1')  

        # all non valid
        ii                  = np.where(valid_bool)
        valid_point_num     = len(ii)
        if valid_point_num < 5:
            return None
        step_size           = np.maximum(1, np.int32(valid_point_num/point_num))
        ii                  = ii[::step_size]

        # plane params - using only valid
        z                   = img_roi[ii]
        xyz_matrix          = self.matrix_dir[ii,:]
        xyz_matrix[:,:3]    = xyz_matrix[:,:3]*z  # keep 1 intact

        # update corners of the rect in 3d
        #self.rect_3d        = self.matrix_dir[self.corner_ind,:]*img_roi[self.corner_ind]

        # substract mean
        #xyz_center          = xyz_matrix[:,:3].mean(axis=0)
        #xyz_matrix          = xyz_matrix - xyz_center   
        #log.info(f'Timing : 2')     

        # mtrx_dir            = np.hstack((self.matrix_dir[valid_bool,0]*z,self.matrix_dir[valid_bool,1]*z,z*0+1))
        # mtrx_inv            = np.linalg.pinv(mtrx_dir)
        # #mtrx_inv            = self.matrix_inv[:,valid_bool]
        # plane_params        = np.dot(mtrx_inv,z)

        # decimate to make it run faster  reduce size of the grid for speed. 1000 pix - 30x30 - step 1, 10000 pix - step=3
        #roi_area            = n*m
        #step_size           = int(np.sqrt(roi_area)/7) if roi_area > 1000 else 1  

        #self.plane_center   = xyz_center.flatten()              

        return xyz_matrix

    def convert_plane_params(self, plane_equation):
        "convert plane params to rvec"
        # 4. Convert plane parameters to rvec and tvec
        #    - The plane normal vector is (A, B, C).
        #    - We can use the normal vector to get the rotation.
        #    - A point on the plane can be used for the translation vector.

        # Normalize the plane normal vector
        normal      = plane_equation #np.array([plane_equation[0], plane_equation[1], plane_equation[2]])
        normal_norm = np.linalg.norm(normal)
        if normal_norm == 0:
            log.error("Error: Zero norm for plane normal vector.")
            return None
        normal = normal / normal_norm

        # Use the normalized normal vector to get the rotation matrix
        # This is a common method, but there are other ways to do this.
        z_axis        = np.array([0, 0, 1])
        rotation_axis = np.cross(z_axis, normal)
        rotation_angle = np.arccos(np.dot(z_axis, normal))

        # Handle the case where the rotation axis is zero (normal is parallel to z-axis)
        if np.linalg.norm(rotation_axis) < 1e-6:
            if normal[2] > 0:
                rvec = np.zeros(3)  # Rotation is identity
            else:
                rvec = np.array([0, np.pi, 0]) # Rotation by 180 degrees around X or Y.
        else:
            rvec, _ = cv.Rodrigues(rotation_axis * rotation_angle)

        return rvec

    def convert_plane_params_to_pose(self, plane_params = None, plane_center = None):
        "converting params of the plane to the pose vector"

        plane_params = self.plane_params if plane_params is None else plane_params[:3].flatten()
        plane_center = self.plane_center if plane_center is None else plane_center[:3].flatten()

        tvec       = plane_center.reshape((1,-1))
        rvec       = plane_params.reshape((1,-1)) #reshape((-1,1))
        rvec       = rvec/np.linalg.norm(rvec.flatten())

        pose_norm  = np.hstack((tvec, rvec))
        #log.info('roi to pose')
        return pose_norm #.flatten()

    def fit_plane_init(self):
        "prepares data for real time fit a*x+b*y+c = z"
        self.cam_matrix   = np.array([[650,0,self.frame_size[0]/2],[0,650,self.frame_size[1]/2],[0,0,1]], dtype = np.float32)
        self.cam_distort  = np.array([0,0,0,0,0],dtype = np.float32)

        x0,y0,x1,y1     = self.rect 
        h,w             = y1-y0, x1-x0
        x_grid          = np.arange(x0, x1, 1)
        y_grid          = np.arange(y0, y1, 1)
        x, y            = np.meshgrid(x_grid, y_grid)  

        # remember corner indexes for reprojection [0 .... h*(w-1))
        #                                           .        .
        #                                           h ......h*w-1]
        #self.corner_ind = [0, h,  h*w-1, h*(w-1), 0]
        h2,w2           = h>>1, w>>1
        self.rect_3d    = [[-w,-h,0],[w,-h,0],[w,h,0],[-w,h,0],[-w,-h,0]]

        # camera coordinates
        xy              = np.hstack((x.reshape(-1,1),y.reshape(-1,1)))
        xy              = np.expand_dims(xy, axis=1).astype(np.float32)
        xy_undistorted  = cv.undistortPoints(xy, self.cam_matrix, self.cam_distort)

        u               = xy_undistorted[:,0,0].reshape((h,w)).reshape(-1,1)
        v               = xy_undistorted[:,0,1].reshape((h,w)).reshape(-1,1)

        # check
        #u, v            = u*self.cam_matrix[0,0], v*self.cam_matrix[1,1]

        self.matrix_dir = np.hstack((u,v,u*0+1))
        self.matrix_inv = np.linalg.pinv(self.matrix_dir)

    def fit_plane_svd(self, img_roi):
        "estimates mean and std of the plane fit"
        n,m             = img_roi.shape[:2]
        img_roi         = img_roi.reshape((-1,1))
        valid_bool      = img_roi > 0
        valid_bool      = valid_bool.flatten()
        #log.info(f'Timing : 1')  

        # init params of the inverse
        if self.matrix_inv is None:
            self.fit_plane_init()

        # plane params - using only valid
        z                   = img_roi[valid_bool]
        xyz_matrix          = self.matrix_dir[valid_bool,:]
        xyz_matrix[:,:3]    = xyz_matrix[:,:3]*z  # keep 1 intact

        # update corners of the rect in 3d
        #self.rect_3d        = self.matrix_dir[self.corner_ind,:]*img_roi[self.corner_ind]

        # substract mean
        xyz_center          = xyz_matrix[:,:3].mean(axis=0)
        xyz_matrix          = xyz_matrix - xyz_center   
        #log.info(f'Timing : 2')     

        # mtrx_dir            = np.hstack((self.matrix_dir[valid_bool,0]*z,self.matrix_dir[valid_bool,1]*z,z*0+1))
        # mtrx_inv            = np.linalg.pinv(mtrx_dir)
        # #mtrx_inv            = self.matrix_inv[:,valid_bool]
        # plane_params        = np.dot(mtrx_inv,z)

        # decimate to make it run faster  reduce size of the grid for speed. 1000 pix - 30x30 - step 1, 10000 pix - step=3
        roi_area            = n*m
        step_size           = int(np.sqrt(roi_area)/7) if roi_area > 1000 else 1
        
        # using svd to make the fit
        U, S, Vh            = np.linalg.svd(xyz_matrix[::step_size,:], full_matrices=True)
        ii                  = np.argmin(S)
        vnorm               = Vh[ii,:]
        #log.info(f'Timing : 3') 

        # keep orientation
        plane_params       = vnorm*np.sign(vnorm[2])

        # estimate error
        err                = np.dot(xyz_matrix,plane_params)
        z_est              = z + err + xyz_center[2]

        img_mean           = xyz_center[2] #z_est.mean()
        img_std            = err.std()
        self.plane_params  = plane_params[:3].flatten()
        self.plane_center  = xyz_center.flatten()

        #log.info(f'Plane : {self.plane_params}, error {img_std:.3f}, step {step_size}')
        
        return img_mean, img_std  
    
    def fit_plane_with_outliers(self, img_roi):
        "computes normal for the specifric roi and evaluates error. Do it twice to reject outliers"

        # roi converted to points with step size on the grid
        xyz_matrix  = self.convert_roi_to_points(img_roi, point_num = 50, step_size = 0)

        # substract mean
        xyz_center   = xyz_matrix[:,:3].mean(axis=0)
        xyz_matrix   = xyz_matrix - xyz_center         

        # using svd to make the fit to a sub group     
        U, S, Vh    = np.linalg.svd(xyz_matrix, full_matrices=True)
        ii          = np.argmin(S)
        vnorm       = Vh[ii,:]
        #vnorm       = vnorm*np.sign(vnorm[2]) # keep orientation

        # keep orientation
        plane_params = vnorm*np.sign(vnorm[2])

        # estimate error
        err         = np.dot(xyz_matrix,plane_params)        
        err_std     = err.std()
        log.info('Fit error iteration 1: %s' %str(err_std))

        # filter only the matching points
        inlier_ind  = np.abs(err) < 5*err_std

        # perform svd one more time 
        tvec        = xyz_matrix[inlier_ind,:3].mean(axis=0)#  
        xyz1        = xyz_matrix[inlier_ind,:] - tvec 
        U, S, Vh    = np.linalg.svd(xyz1, full_matrices=True)
        ii          = np.argmin(S)
        vnorm       = Vh[ii,:]
        # keep orientation
        plane_params = vnorm*np.sign(vnorm[2])

        # checking error
        err         = np.dot(xyz1, plane_params)
        err_std     = err.std()
        log.info('Fit error iteration 2: %s' %str(err_std))    

        # We can convert this flat index to row and column indices
        row_index, col_index = np.unravel_index(inlier_ind, self.img_mask.shape)
        self.img_mask[row_index, col_index] = 1    

        img_mean           = tvec[2] #z_est.mean()
        img_std            = err.std()
        self.plane_params  = plane_params[:3].flatten()
        self.plane_center  = tvec.flatten()

        #log.info(f'Plane : {self.plane_params}, error {img_std:.3f}, step {step_size}')
        
        return img_mean, img_std   
    
    def fit_plane_ransac(self, roi):
        
        """
        Find the best equation for a plane.

        :param pts: 3D point cloud as a `np.array (N,3)`.
        :param thresh: Threshold distance from the plane which is considered inlier.
        :param maxIteration: Number of maximum iteration which RANSAC will loop over.
        :returns:
        - `self.equation`:  Parameters of the plane using Ax+By+Cy+D `np.array (1, 4)`
        - `self.inliers`: points from the dataset considered inliers

        """
        log.info('Fit ransac: ...')  
        # roi converted to points with step size on the grid
        pts            = self.convert_roi_to_points(step_size = 0)

        thresh         = 0.05
        minPoints      = 100
        maxIteration   = 100

        import random
        n_points        = pts.shape[0]
        best_eq         = []
        best_inliers    = []

        for it in range(maxIteration):

            # Samples 3 random points
            id_samples = random.sample(range(0, n_points), 3)
            pt_samples = pts[id_samples]

            # We have to find the plane equation described by those 3 points
            # We find first 2 vectors that are part of this plane
            # A = pt2 - pt1
            # B = pt3 - pt1

            vecA = pt_samples[1, :] - pt_samples[0, :]
            vecB = pt_samples[2, :] - pt_samples[0, :]

            # Now we compute the cross product of vecA and vecB to get vecC which is normal to the plane
            vecC = np.cross(vecA, vecB)

            # The plane equation will be vecC[0]*x + vecC[1]*y + vecC[0]*z = -k
            # We have to use a point to find k
            vecC = vecC / np.linalg.norm(vecC)
            k = -np.sum(np.multiply(vecC, pt_samples[1, :]))
            plane_eq = [vecC[0], vecC[1], vecC[2], k]

            # Distance from a point to a plane
            # https://mathworld.wolfram.com/Point-PlaneDistance.html
            pt_id_inliers = []  # list of inliers ids
            dist_pt = (
                plane_eq[0] * pts[:, 0] + plane_eq[1] * pts[:, 1] + plane_eq[2] * pts[:, 2] + plane_eq[3]
            ) / np.sqrt(plane_eq[0] ** 2 + plane_eq[1] ** 2 + plane_eq[2] ** 2)

            # Select indexes where distance is biggers than the threshold
            pt_id_inliers = np.where(np.abs(dist_pt) <= thresh)[0]
            if len(pt_id_inliers) > len(best_inliers):
                best_eq = plane_eq
                best_inliers = pt_id_inliers
        
        self.inliers = best_inliers
        self.equation = best_eq

        # rtansform to pose output
        tvec     = pts[best_inliers,:].mean(axis=0)
        pts_best = pts[best_inliers,:] - tvec
        vnorm    = np.array(best_eq[:3])

        # checking error
        err         = np.dot(pts_best, vnorm)
        err_std     = err.std()
        log.info('Fit error ransac: %s' %str(err_std))  

        # forming output
        roi_params  = {'roi':best_inliers, 'error': err_std, 'tvec': tvec, 'vnorm':vnorm }          

        return roi_params
    
    def fit_and_split_roi_recursively(self, roi, level = 0):
        # splits ROI on 4 regions and recursevly call 
        x0,y0,x1,y1     = roi
        #roi3d           = self.img_xyz[y0:y1,x0:x1,:]   
        log.info('Processing level %d, region x = %d, y = %d' %(level,x0,y0))
        # check the current fit
        roi_params_f    = self.fit_plane(roi)
        roi_params_ret  = [roi_params_f]
        if roi_params_f['error'] < self.MIN_STD_ERROR:
            log.info('Fit is good enough x = %d, y = %d' %(x0,y0))
            return roi_params_ret

        # too small exit
        xs, ys          = int((x1 + x0)/2), int((y1 + y0)/2)
        if (xs - x0) < self.MIN_SPLIT_SIZE or (ys - y0) < self.MIN_SPLIT_SIZE:
            log.info('Min size is reached x = %d, y = %d' %(x0,y0))
            return roi_params_ret
        
        # 4 ROIs - accept the split if error of one of them is lower from the total
        roi_params_list = []
        roi_split   = [[x0,y0,xs,ys],[x0,ys,xs,y1],[xs,y0,x1,ys],[xs,ys,x1,y1]]
        for roi_s in roi_split:
            roi_params_prev = self.fit_and_split_roi_recursively(roi_s, level + 1)
            # save locally
            #roi_params_list.append(roi_params_prev)
            roi_params_list = roi_params_list + roi_params_prev
            
        # extract each of the below and check the error
        makeTheSplit = False
        for roi_params_s in roi_params_list:
            #roi_params_s       = roi_params_prev[-1]
            # accept the split if twice lower (if noise of 4 split should be 2)
            if roi_params_s['error'] < roi_params_f['error']/2:
                makeTheSplit = True
                break

        # decide what to return
        if makeTheSplit:
            roi_params_ret = roi_params_list
            log.info('Split at level %d, region x = %d, y = %d' %(level,x0,y0))
        else:
            log.info('No split level %d, region x = %d, y = %d' %(level,x0,y0))

        return roi_params_ret

    def find_planes(self, img):
        "finds planes using different algo"

        img_roi             = self.preprocess(img)
        detect_type         = self.detect_type.upper()

        img_mean, img_std   = 0,0             
        if detect_type == 'P':
            img_mean, img_std   = self.fit_plane_svd(img_roi)  
        elif detect_type == 'O':
            img_mean, img_std   = self.fit_plane_with_outliers(img_roi)  
        elif detect_type == 'R':
            img_mean, img_std   = self.fit_plane_ransac(img_roi)              
            
        #log.debug(f'camera noise           - roi mean : {img_mean}')
        self.img_mean       = img_mean        # final measurements per frame
        self.img_std        = img_std    
        return True 

    def process_frame(self, img):
        "process the entire image and find the planes"

        img_roi     = self.preprocess(img)
        img3d       = self.init_img3d(img_roi)
        imgXYZ      = self.compute_img3d(img_roi)
        roim,rois   = self.fit_plane_with_outliers(img_roi)
        pose        = self.convert_plane_params_to_pose()

        return pose
        
if __name__ == '__main__':
    #print(__doc__)
    p = PlaneDetector()




