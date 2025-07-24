
'''
Noise Measurement App - Tester
==================

Using depth image to measure noise in a specific selected ROI.

Usage
-----
 Run NoiseApp - it will connect to the RealSense camera.
 Select different measurement options:
 - Press '
 Manually select ROI 


Environemt : 
-----
    .\\Envs\\barcode

Install : 


'''
import sys
import numpy as np
import cv2 as cv
import matplotlib.pyplot as plt

# importing common Use modules 
sys.path.append(r'wrappers\python\applications\utils\src')
from opencv_realsense_camera import RealSense
from camera_noise_estimator import NoiseEstimator
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

def draw_str(dst, target, s):
    x, y = target
    dst = cv.putText(dst, s, (x+1, y+1), cv.FONT_HERSHEY_PLAIN, 1.0, (0, 0, 0), thickness = 2, lineType=cv.LINE_AA)
    dst = cv.putText(dst, s, (x, y), cv.FONT_HERSHEY_PLAIN, 1.0, (255, 255, 255), lineType=cv.LINE_AA)
    return dst


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

#%% Help function to display results
class DataDisplay:
    def __init__(self):
        self.tracker        = None

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
            print("Error: Zero norm for plane normal vector.")
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

    def show_axis(self, vis):
        "draw axis after plane estimation"
        if self.tracker.plane_params is None:
            return vis
        
        #rvec = self.plane_params/np.sum(self.plane_params**2) # normalize
        rvec = self.convert_plane_params(self.tracker.plane_params)
        #rvec = self.plane_to_rvec(self.plane_params)
        
        tvec = self.tracker.plane_center
        vis  = draw_axis(vis, rvec, tvec, self.tracker.cam_matrix, self.tracker.cam_distort, len = 10)
        return vis
    
    def show_rect_and_text(self, vis):
        "draw axis after plane estimation"
        err_mean, err_std = self.tracker.img_mean, self.tracker.img_std
        if err_mean is None:
            return vis

        
        measure_type = self.tracker.measure_type
        img_fill     = self.tracker.img_fill
        x0, y0, x1, y1 = self.tracker.rect
        
        clr = (0, 0, 0) if vis[y0:y1,x0:x1].mean() > 128 else (240,240,240)
        vis = cv.rectangle(vis, (x0, y0), (x1, y1), clr, 2)
        txt = f'{measure_type}:{err_mean:.2f}-{err_std:.3f}'
        if measure_type == 'F':
            txt = f'{measure_type}:{img_fill:.2f} %'
        vis = draw_str(vis,(x0,y0-10),txt)

        return vis    

    def show_scene(self, tracker, vis):
        "draw ROI and Info"
        self.tracker    = tracker
        vis             = self.show_rect_and_text(vis)
        vis             = self.show_axis(vis)

        return vis     
    
    def show_measurements(self, depth_true = None):
        "show roi statistics for all frames"
        image_num = len(self.depth_mean)
        if image_num < 2:
            #log.info('No stat data is collected')
            return 
        
        d_mean   = np.array(self.depth_mean)
        d_std    = np.array(self.depth_std)
        t_frame  = np.arange(image_num)

        # true data is set
        x_axis_is_depth = False
        if depth_true is not None:
            t_frame[:len(depth_true)] = np.array(depth_true)
            x_axis_is_depth = True
            #print(d_mean)
            #print(t_frame)
            d_mean = d_mean - t_frame


        # fig, ax = plt.subplots()
        # fig.set_size_inches([24, 16])
        # ax.set_title('Depth Noise vs Distance')
        # ax.set_xlabel('Frame [#]')
        # ax.set_ylabel('Distance [mm]')
        # ax.scatter(t_frame, d_mean,       c='b', lw=3, label='mean')
        # ax.scatter(t_frame, d_mean+d_std, c='r', lw=1, label='mean+std')
        # ax.scatter(t_frame, d_mean-d_std, c='g', lw=1, label='mean-std')
        # ax.legend()
        # plt.show()

        fig, ax = plt.subplots()
        fig.set_size_inches([24, 16])
        ax.set_title('Depth Noise vs Distance')
        if x_axis_is_depth:
            ax.set_xlabel('Distance [mm]')
        else:        
            ax.set_xlabel('Frame [#]')

        ax.set_ylabel('Distance Error [mm]')
        ax.scatter(t_frame, d_mean,  c='r', lw=3, label='mean err')
        ax.scatter(t_frame, d_mean-d_std,  c='b', lw=3, label='mean err-std')
        ax.scatter(t_frame, d_mean+d_std,  c='g', lw=3, label='mean err+std')
        ax.legend()
          

        if x_axis_is_depth:
            fig, ax = plt.subplots()
            fig.set_size_inches([24, 16])
            ax.set_title('Depth Noise vs Distance')
            ax.set_xlabel('Distance [mm]')
            ax.set_ylabel('Distance Error [mm]')
            ax.plot(t_frame, d_mean,  '-o', label='mean')        
            ax.legend()
        
        
        plt.show()              

        return    
      

#%% App that uses camera and Noise estimator
class NoiseApp:
    def __init__(self):
        self.cap            = RealSense() #
        self.cap.set_display_mode('iid')
        self.frame          = None
        self.rect           = None
        self.paused         = False
        self.trackers       = []
        self.display        = DataDisplay()

        self.show_dict     = {} # hist show
        self.measure_type  = 'T'
        self.win_name      = 'Noise (q-quit, a,f,p,s,t)'

        cv.namedWindow(self.win_name )
        self.rect_sel = RectSelector(self.win_name , self.on_rect)

    def define_roi(self, image):
        '''define ROI target.'''
        h,w            = image.shape[:2]
        rect           = [0, 0, w, h]
        return rect

    def on_rect(self, rect):
        "remember ROI defined by user"
        #self.define_roi(self.frame, rect)
        tracker             = NoiseEstimator() #estimator_type=self.estim_type, estimator_id=estim_ind)
        tracker.rect        = rect
        tracker.measure_type = self.measure_type
        self.trackers.append(tracker)        
        log.info(f'Adding noise estimator at  : {rect}') 

    def process_image(self, img_depth):
        "makes measurements"
        for tracker in self.trackers:
            tracker.do_measurements(img_depth) 

    def show_scene(self, frame):
        "draw ROI and Info"
        vis     = np.uint8(frame.copy())
        vis     = cv.cvtColor(vis, cv.COLOR_GRAY2BGR)
        #vis     = cv.convertScaleAbs(self.frame, alpha=0.03)
        self.rect_sel.draw(vis)

        for tracker in self.trackers:
            vis = self.display.show_scene(tracker, vis)

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

            img_depth = self.frame[:,:,2]    


            #self.statistics(frame)
            self.process_image(img_depth)

            #self.show_histogram(frame)

            vis     = self.show_scene(img_depth)


            cv.imshow(self.win_name , vis)
            ch = cv.waitKey(1)
            if ch == ord(' '):
                self.paused = not self.paused
            elif ch == ord('a'):
                self.measure_type = 'A' 
            elif ch == ord('f'):
                self.measure_type = 'F'  
            elif ch == ord('p'):
                self.measure_type = 'P'  
            elif ch == ord('s'):
                self.measure_type = 'S'      
            elif ch == ord('t'):
                self.measure_type = 'T'                                               
            if ch == ord('c'):
                if len(self.trackers) > 0:
                    t = self.trackers.pop()
            if ch == 27 or ch == ord('q'):
                break

if __name__ == '__main__':
    #print(__doc__)
    NoiseApp().run()