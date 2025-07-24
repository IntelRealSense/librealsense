
'''
Noise Measurement App
==================

Using depth image to measure noise in specific image ROI.

Usage
-----

Environemt : 
    .\\Envs\\barcode

Install : 


'''

import sys 
import numpy as np
import cv2 as cv

# importing common Use modules 
sys.path.append(r'..\utils\src')
from logger import log


#%% Main Function
class NoiseEstimator:
    def __init__(self, m_type = 'T'):
        self.frame          = None
        self.rect           = None
        self.img_int_mean   = None
        self.img_int_std    = None  
        self.roi_type       = 0        # 0-entire image
        self.measure_type   = m_type   # type of the measurements

        # plane fit params
        self.frame_size     = (1280,720)
        self.matrix_inv     = None     # holds inverse params of the 
        self.matrix_dir     = None     # direct u,v,1
        self.plane_params   = None     # rvec not normalized
        self.plane_center   = None     # tvec
        self.cam_matrix     = None
        self.cam_distort    = None

        # statistics
        self.depth_mean     = []       # vector of mean measurements of ROI 
        self.depth_std      = []       # vector of std measurements of ROI 
        self.depth_true     = []       # actual depth values    

        self.img_mean       = 0        # final measurements per frame
        self.img_std        = 0     
        self.img_fill       = 0        # fill rate


    def init_roi(self, frame, test_type = 14):
        "load the roi case"
        h,w     = frame.shape[:2]
        w2, h2  = w>>1, h>>1
        roi     = [0,0,w,h]
        if test_type == 1:
            roi = [w2-3,h2-3,w2+3,h2+3] # xlu, ylu, xrb, yrb
        elif test_type == 2:
            roi = [300,220,340,260] # xlu, ylu, xrb, yrb
        elif test_type == 3:
            roi = [280,200,360,280] # xlu, ylu, xrb, yrb            
        elif test_type == 4:
            roi = [220,140,420,340] # xlu, ylu, xrb, yrb      
        elif test_type == 5:
            roi = [200,120,440,360] # xlu, ylu, xrb, yrb    
        elif test_type == 11:
            roi = [w2-16,h2-16,w2+16,h2+16] # xlu, ylu, xrb, yrb             
        elif test_type == 12:
            roi = [w2-32,h2-32,w2+32,h2+32] # xlu, ylu, xrb, yrb    
        elif test_type == 13:
            roi = [w2-64,h2-64,w2+64,h2+64] # xlu, ylu, xrb, yrb      
        elif test_type == 14:
            roi = [w2-64,h2-48,w2+64,h2+48] # xlu, ylu, xrb, yrb       

        log.info(f'Using ROI : {roi}')                           
        return roi         

    def preprocess(self, img):
        "image preprocessing - extracts roi and converts from uint8 to float using log function"
        if self.rect is None: # use entire image
            self.rect = self.init_roi(img, self.roi_type)
            
        x0, y0, x1, y1  = self.rect
        if len(img.shape) > 2:
            img_roi        = img[y0:y1,x0:x1,2].astype(np.float32)
        else:
            img_roi        = img[y0:y1,x0:x1].astype(np.float32)
        return img_roi          

    def temporal_noise(self, img_roi):
        "makes integration over ROI and estimates STD of the region"
        assert len(img_roi.shape) < 3 , 'image must be 2D - depth only'

        if self.img_int_mean is None:
            self.img_int_mean = img_roi
            self.img_int_std  = np.zeros_like(img_roi)

        # reset if the shape is changed
        if not np.all(img_roi.shape == self.img_int_mean.shape):
            self.img_int_mean = img_roi
            self.img_int_std  = np.zeros_like(img_roi)            
        
        valid_bool        = img_roi > 0
        #valid_num         = valid_bool.sum()
        #nr,nc             = img_roi.shape[:2]

        self.img_int_mean += 0.1*(img_roi - self.img_int_mean)
        self.img_int_std  += 0.1*(np.abs(img_roi - self.img_int_mean) - self.img_int_std)

        err_std_valid      = self.img_int_std.copy()
        err_std_valid[~valid_bool]    = 0 #100
        err_std            = err_std_valid.mean()
        err_mean           = self.img_int_mean.mean()
        return err_mean, err_std   
    
    def spatial_noise_parallel(self, img_roi):
        "estimates mean and std of the flat surface - parallel to the camera"
        #assert len(img_roi.shape) < 3 , 'image must be 2D - depth only'
        
        valid_bool         = img_roi > 0

        self.img_int_mean  = img_roi[valid_bool].mean()
        self.img_int_std   = img_roi[valid_bool].std()

        img_mean           = self.img_int_mean
        img_std            = self.img_int_std
        return img_mean, img_std     
    
    def plane_fit_init(self):
        "prepares data for real time fit a*x+b*y+c = z"
        self.cam_matrix   = np.array([[650,0,self.frame_size[0]/2],[0,650,self.frame_size[1]/2],[0,0,1]], dtype = np.float32)
        self.cam_distort  = np.array([0,0,0,0,0],dtype = np.float32)

        x0,y0,x1,y1     = self.rect 
        h,w             = y1-y0, x1-x0
        x_grid          = np.arange(x0, x1, 1)
        y_grid          = np.arange(y0, y1, 1)
        x, y            = np.meshgrid(x_grid, y_grid)  

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
    
    def plane_fit_noise(self, img_roi):
        "estimates mean and std of the plane fit"
        n,m             = img_roi.shape[:2]
        img_roi         = img_roi.reshape((-1,1))
        valid_bool      = img_roi > 0
        valid_bool      = valid_bool.flatten()
        #log.info(f'Timing : 1')  

        # init params of the inverse
        if self.matrix_inv is None:
            self.plane_fit_init()

        # plane params - using only valid
        z                   = img_roi[valid_bool]
        xyz_matrix          = self.matrix_dir[valid_bool,:]
        xyz_matrix[:,:3]    = xyz_matrix[:,:3]*z  # keep 1 intact
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
        z_est              = z + err

        img_mean           = z_est.mean()
        img_std            = err.std()
        self.plane_params  = plane_params[:3].flatten()
        self.plane_center  = xyz_center.flatten()

        log.info(f'Plane : {self.plane_params}, error {img_std:.3f}, step {step_size}')
        
        return img_mean, img_std     

    def estimate_fill_rate(self, img_roi):
        "estimates mean and std of the flat surface - parallel to the camera"
        #assert len(img_roi.shape) < 3 , 'image must be 2D - depth only'
        
        non_valid_bool     = img_roi < 1
        m,n                = img_roi.shape[:2]
        fill_rate          = non_valid_bool.sum()/(m*n)

        return (1 - fill_rate) # valid fill

    def process_image(self, img):
        "makes integration and noise measurement over ROI"

        img_roi             = self.preprocess(img)
        #err_mean, err_std   = self.temporal_noise(img_roi)
        img_mean, img_std   = self.spatial_noise_parallel(img_roi)        

        # this code is used for testing depth fft
        self.depth_mean.append(img_mean)
        self.depth_std.append(img_std) 

        #log.debug(f'camera noise           - roi mean : {img_mean}')

        return img_std
    
    def do_measurements(self, img):
        "makes integration and noise measurement over ROI"

        img_roi             = self.preprocess(img)
        measure_type        = self.measure_type.upper()

        img_mean, img_std, fill_rate = 0,0,0
        if  measure_type == 'T':
            img_mean, img_std   = self.temporal_noise(img_roi)
        elif measure_type == 'S':
            img_mean, img_std   = self.spatial_noise_parallel(img_roi)      
        elif measure_type == 'F':
            fill_rate           = self.estimate_fill_rate(img_roi)                
        elif measure_type == 'P':
            img_mean, img_std   = self.plane_fit_noise(img_roi)  
        elif measure_type == 'A':
            ret                 = self.show_statistics(img_roi)  
        #log.debug(f'camera noise           - roi mean : {img_mean}')
        self.img_mean       = img_mean        # final measurements per frame
        self.img_std        = img_std   
        self.img_fill       = fill_rate  
        return True  
 
    def print_statistics(self, frame):
        "just print some info"
        ii  = np.logical_and(frame > 0.5, frame < 15000) # 65535 - None

        isok = np.any(np.isfinite([frame[ii]]))
        minv = frame[ii].min()
        maxv = frame[ii].max()
        avg  = frame[ii].mean()
        stdv = frame[ii].std()
        locv = ii.sum()  #np.any(np.isnan(frame[ii]))
        log.info(f'Stat : {minv:.2f}:{avg:.2f}:{maxv:.2f} - std {stdv:.2f}. Has None {locv}, is finite {isok}')   
        return True

if __name__ == '__main__':
    #print(__doc__)
    m = NoiseEstimator()