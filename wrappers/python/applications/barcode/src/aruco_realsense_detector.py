# -*- coding: utf-8 -*-
#cython: language_level=3
#distutils: language=c++

"""
Aruco Marker Detection

-----------------------------
 Ver    Date     Who    Descr
-----------------------------
2106   05.07.24 UD     Suppoprt old version for compile. Bug fix
1911   11.02.24 UD     Aruco detector.
-----------------------------
"""


import cv2
import numpy as np
import copy



pattern_size = (9,6)
square_size = 10.5

ARUCO_DICT = {
    "DICT_4X4_50": cv2.aruco.DICT_4X4_50,
    "DICT_4X4_100": cv2.aruco.DICT_4X4_100,
    "DICT_4X4_250": cv2.aruco.DICT_4X4_250,
    "DICT_4X4_1000": cv2.aruco.DICT_4X4_1000,
    "DICT_5X5_50": cv2.aruco.DICT_5X5_50,
    "DICT_5X5_100": cv2.aruco.DICT_5X5_100,
    "DICT_5X5_250": cv2.aruco.DICT_5X5_250,
    "DICT_5X5_1000": cv2.aruco.DICT_5X5_1000,
    "DICT_6X6_50": cv2.aruco.DICT_6X6_50,
    "DICT_6X6_100": cv2.aruco.DICT_6X6_100,
    "DICT_6X6_250": cv2.aruco.DICT_6X6_250,
    "DICT_6X6_1000": cv2.aruco.DICT_6X6_1000,
    "DICT_7X7_50": cv2.aruco.DICT_7X7_50,
    "DICT_7X7_100": cv2.aruco.DICT_7X7_100,
    "DICT_7X7_250": cv2.aruco.DICT_7X7_250,
    "DICT_7X7_1000": cv2.aruco.DICT_7X7_1000,
    "DICT_ARUCO_ORIGINAL": cv2.aruco.DICT_ARUCO_ORIGINAL,
    "DICT_APRILTAG_16h5": cv2.aruco.DICT_APRILTAG_16h5,
    "DICT_APRILTAG_25h9": cv2.aruco.DICT_APRILTAG_25h9,
    "DICT_APRILTAG_36h10": cv2.aruco.DICT_APRILTAG_36h10,
    "DICT_APRILTAG_36h11": cv2.aruco.DICT_APRILTAG_36h11
}

# Things to draw objects
def draw_axis(img, imgpts, objid):
   
    imgpts = imgpts.astype(np.int32)
    fs, ft, ff, fl = 0.9, 2, cv2.FONT_HERSHEY_SIMPLEX, 10

    origin = tuple(imgpts[0].ravel())
    img = cv2.arrowedLine(img, origin, tuple(imgpts[1].ravel()), (0,0,255), 5)
    img = cv2.arrowedLine(img, origin, tuple(imgpts[2].ravel()), (0,255,0), 5)
    img = cv2.arrowedLine(img, origin, tuple(imgpts[3].ravel()), (255,0,0), 5)

    
    cv2.putText(img ,'X',tuple(fl + imgpts[1].ravel()), ff, fs,(0,0,255),ft,cv2.LINE_AA)
    cv2.putText(img ,'Y',tuple(fl + imgpts[2].ravel()), ff, fs,(0,255,0),ft,cv2.LINE_AA)
    cv2.putText(img ,'Z',tuple(fl + imgpts[3].ravel()), ff, fs,(255,0,0),ft,cv2.LINE_AA)

    cv2.putText(img ,str(int(objid)),tuple(fl + imgpts[0].ravel()), ff, fs,(255,255,255),ft,cv2.LINE_AA)
   
    return img

#%% Main
class ObjectAruco:
    """ 
    The object part that need to be detected and estimate its pose
    """

    def __init__(self):
        
        self.name          = 'aruco'

        #self.estimate6d    = objpose.ObjPose()

        self.pattern_points = []
        self.init_pattern(pattern_size, square_size)
        
        self.camera_matrix  = np.array([[1000.0,0.0,640.0],[0,1000.0,360.0],[0,0,1.0]])
        self.dist_coeffs    = np.zeros((1,5))
        self.marker_length  = 100  # mm marker size
        self.scale_factor   = 1   # image scaling
        
        self.aruco_type      = ARUCO_DICT["DICT_6X6_250"]
        self.corners         = [] # aruco corners array
        self.ids             = [] # aruco ids array  
        
        self.debugOn        = False
        
        
        
    def init(self, pose_cfg = None):
        """
        initialized for detection

        """

        global pattern_size
        p_size              = self.cfg.get('chess_board_size')
        if p_size is not None:
            pattern_size     = p_size
        
        sq_size             =  np.array(self.cfg['square_size'])
        self.init_pattern(square_size = sq_size)
        
        # check cfg is initialized
        self.camera_matrix  = np.array(self.cfg['CamMatrix'])
        self.dist_coeffs    = np.array(self.cfg['CamDist'])
        
        stype                = self.cfg.get('aruco_type')
        self.aruco_type      = ARUCO_DICT["DICT_6X6_250"] if stype is None else ARUCO_DICT[str(stype)]
        
        slen                 = self.cfg.get('aruco_side_length')
        self.marker_length    = 10 if slen is None else np.array(slen)
        
        sfact                 = self.cfg.get('scale_factor')
        self.scale_factor     = 1 if sfact is None else np.array(sfact)
        self.Print('Configured parameters : Aruco type %s and size %s. Scale Factor %s' %(str(self.aruco_type),str(self.marker_length),str(self.scale_factor)))
         
    
    def detect_aruco_pose(self, rgb_image):
        
        """
        detect multiple aruco 
        
        """
        
        # select object to work with
        objects = {'objectId': [], 'rvecAll': [], 'tvecAll': [], 'objectQ': []}
         
        corners, ids, rejected                = self.detect_aruco(rgb_image)
        self.corners                          = corners
        self.ids                              = ids
        aruco_num                             = len(corners)
        if aruco_num < 1:
            #self.Print("Failed to find marker" , 'W')
            return objects
        
        # flatten the ArUco IDs list
        #ids = ids.flatten()
        # loop over the detected ArUCo corners
        objectId, rvecAll, tvecAll, objectQ = [], [], [], []
        for (markerCorner, markerID) in zip(corners, ids.flatten()):
            # extract the marker corners (which are always returned in
            # top-left, top-right, bottom-right, and bottom-left order)
            corners       = markerCorner.reshape((4, 2))
            (topLeft, topRight, bottomRight, bottomLeft) = corners
            # convert each of the (x, y)-coordinate pairs to integers
            topRight      = (int(topRight[0]), int(topRight[1]))
            bottomRight   = (int(bottomRight[0]), int(bottomRight[1]))
            bottomLeft    = (int(bottomLeft[0]), int(bottomLeft[1]))
            topLeft       = (int(topLeft[0]), int(topLeft[1]))
            
            centerX       = (topRight[0] + bottomRight[0] + bottomLeft[0] + topLeft[0]) / 4
            centerY       = (topRight[1] + bottomRight[1] + bottomLeft[1] + topLeft[1]) / 4
            
            # adjust things
            mark_length   = self.marker_length
            mark_corners  = markerCorner/self.scale_factor
            
            # marker length in meters
            rvec, tvec, _points    = cv2.aruco.estimatePoseSingleMarkers(mark_corners, mark_length, self.camera_matrix, self.dist_coeffs)
            
            # diagonal length have length more than 50 is ok
            quality       = np.minimum(1,((topRight[1] - topLeft[1])**2 + (topRight[0] - topLeft[0])**2) / 2500)
            
            objectId.append(markerID)
            rvecAll.append(rvec.reshape((3,1)))
            #tvecAll.append(np.array([centerX,centerY,0]).reshape((3,1)))
            tvecAll.append(tvec.reshape((3,1)))
            objectQ.append(quality)
            
            

        # save to output
        objects['objectId']            = objectId
        objects['rvecAll']             = rvecAll
        objects['tvecAll']             = tvecAll
        objects['objectQ']             = objectQ  # reliable
                
        return objects        
    
    
    def detect_aruco(self, frame_rgb, objectsInfo = None):
        # aruco detection according to the type

        img             = frame_rgb
        aruco_type      = self.aruco_type
        
        # support old version of open cv
        vers            = cv2.__version__.split('.')
        if int(vers[1]) < 6:
            arucoDict       = cv2.aruco.Dictionary_get(aruco_type)
            arucoParams     = cv2.aruco.DetectorParameters_create()
            #(corners, ids, rejected) = cv2.aruco.detectMarkers(img, arucoDict, parameters=arucoParams)
        
        
        #dictionary      = cv2.aruco.getPredefinedDictionary(cv.aruco.DICT_4X4_250)
        #arucoParams     =  cv2.aruco.DetectorParameters()
        #detector = cv.aruco.ArucoDetector(dictionary, parameters)
        else:

            arucoDict       = cv2.aruco.getPredefinedDictionary(aruco_type)
            arucoParams     = cv2.aruco.DetectorParameters()
        
        (corners, ids, rejected) = cv2.aruco.detectMarkers(img, arucoDict, parameters=arucoParams)

        #self.Print('%d arucos detected' %len(corners))

        return corners, ids, rejected    
    
    
    def init_pattern(self, pattern_size = (9,6), square_size = 10.5):
        # chessboard pattern init
        self.pattern_points = np.zeros( (np.prod(pattern_size), 3), np.float32 )
        self.pattern_points[:,:2] = np.indices(pattern_size).T.reshape(-1, 2)
        self.pattern_points *= square_size
        
    def draw_markers(self, color_image):
        # show image with markers previously detected
        corners, ids = self.corners, self.ids
        corners      = corners/self.scale_factor
        
        #color_image  = self.draw_aruco(color_image, 1, corners, ids)
        color_image  = cv2.aruco.drawDetectedMarkers(color_image, corners, ids)       

        return color_image        
    
    def draw_image(self, color_image, corners, ids):
        # show image and barcode
        
        #color_image  = self.draw_aruco(color_image, 1, corners, ids)
        color_image  = cv2.aruco.drawDetectedMarkers(color_image, corners, ids)
        
        #cv2.imshow('Aruco Image',color_image)
        #cv2.waitKey(100)

        return color_image
   
    
    def draw_aruco(self, image, scaleFactor = 1, corners = [], ids = []):
        " draw rectangle and id on the image"
        if len(corners) < 1:
            return image
        # flatten the ArUco IDs list
        ids = ids.flatten()
        # loop over the detected ArUCo corners
        for (markerCorner, markerID) in zip(corners, ids):
            # extract the marker corners (which are always returned in
            # top-left, top-right, bottom-right, and bottom-left order)
            corners     = markerCorner.reshape((4, 2))
            (topLeft, topRight, bottomRight, bottomLeft) = corners
            # convert each of the (x, y)-coordinate pairs to integers
            topRight    = (int(topRight[0]), int(topRight[1]))
            bottomRight = (int(bottomRight[0]), int(bottomRight[1]))
            bottomLeft  = (int(bottomLeft[0]), int(bottomLeft[1]))
            topLeft     = (int(topLeft[0]), int(topLeft[1]))
            
            # draw the bounding box of the ArUCo detection
            cv2.line(image, topLeft, topRight, (0, 255, 0), 2)
            cv2.line(image, topRight, bottomRight, (0, 255, 0), 2)
            cv2.line(image, bottomRight, bottomLeft, (0, 255, 0), 2)
            cv2.line(image, bottomLeft, topLeft, (0, 255, 0), 2)

            # compute and draw the center (x, y)-coordinates of the ArUco
            cX = int((topLeft[0] + bottomRight[0]) / 2.0)
            cY = int((topLeft[1] + bottomRight[1]) / 2.0)
            cv2.circle(image, (cX, cY), 4, (0, 0, 255), -1)

            # draw the ArUco marker ID on the image
            cv2.putText(image, str(markerID), (topLeft[0], topLeft[1] - 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
            print("[INFO] ArUco marker ID: {}".format(markerID))

            # show the output image
#            cv2.imshow("Image", image)
#            cv2.waitKey(0)
              
        # support resize
        return image
    
    def draw_axis_all(self, img, rvecAll, tvecAll):
        "show axis of the aruco"
        cube_size       = 50
        axis_te         = np.float32([[0,0,0],[cube_size,0,0], [0,cube_size,0], [0,0,cube_size]]).reshape(-1,3)          
        ny, nx, nz      = np.shape(img)
        num_to_show     = len(rvecAll)
        mtrx            = self.camera_matrix 
        dist            = self.dist_coeffs
       
        # select object to work with
        for ii in range(num_to_show):
                         
            rvecCO                  = rvecAll[ii]
            tvecCO                  = tvecAll[ii]
            #objQ                    = objectQ[ii]
            objId                   = ii+1
                          
            # show
            imgpts_axis, jac        = cv2.projectPoints(axis_te, rvecCO, tvecCO, mtrx, dist)              
            if ((np.fabs(imgpts_axis) > ny+nx).sum())== 0:    
                   
                # object coordinates
                img                 = draw_axis(img, imgpts_axis, objId)  

        
        return img
    
    def get_object_pose(self, object_points, image_points, camera_matrix, dist_coeffs):
        #ret, rvec, tvec = cv2.solvePnP(object_points, image_points, camera_matrix, dist_coeffs)
        # similarity to object
        ret, rvec, tvec, inliners = cv2.solvePnPRansac(object_points, image_points, camera_matrix, dist_coeffs, flags=cv2.SOLVEPNP_EPNP)

        return rvec.flatten(), tvec.flatten()
 
    
    def Print(self, txt='',level='I'):
        
        if level == 'I':
            ptxt = 'I: ARU: %s' % txt
            #logging.info(ptxt)  
        if level == 'W':
            ptxt = 'W: ARU: %s' % txt
            #logging.warning(ptxt)  
        if level == 'E':
            ptxt = 'E: ARU: %s' % txt
            #logging.error(ptxt)             
        print(ptxt)
    
        

# -----------------------------------------
def test_image():
    import glob
    barcode_list         = []
    img_list            = glob.glob(r'..videos\*.jpg')
    objAruco            = ObjectAruco()
    
    namew                = ' Aruco Demo : q - to quit'
    cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 

    for i, img_name in enumerate(img_list):
        img            = cv2.imread(img_name)
        img            = cv2.resize(img, None, fx=0.5, fy=0.5, interpolation=cv2.INTER_AREA)
        
        objects        = objAruco.detect_aruco_pose(img)
        img            = objAruco.draw_image(img, objAruco.corners, objAruco.ids)
        
        cv2.imshow(namew,   img)       
        ch = cv2.waitKey() & 0xFF
        if ch == ord('q'):
            break 
        
# -------------------------------------------
def test_camera():
   cap                 = cv2.VideoCapture(0)
   cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
   cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)                     

   objAruco            = ObjectAruco()
   
   namew = ' Aruco Demo : q - to quit'
   cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 
   
   while(cap.isOpened()):
       
       ret, img = cap.read()
       if not ret:
           print('The End')
           break    

       objects        = objAruco.detect_aruco_pose(img)
       img            = objAruco.draw_image(img, objAruco.corners, objAruco.ids)
       img            = objAruco.draw_axis_all(img, objects['rvecAll'], objects['tvecAll'] )

       
       cv2.imshow(namew, img)
       ch = cv2.waitKey(10) & 0xFF
       if ch == ord('q'):
           break 

# ----------------------------------------       
def test_video():

    fname               = r'file:///D:/Users/zion/Downloads/fidatu_emulation.avi'
    cap                 = cv2.VideoCapture(fname)
    objAruco            = ObjectAruco()
    
    namew = ' Aruco Demo : q - to quit'
    cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 
    
    while(cap.isOpened()):
        
        ret, img = cap.read()
        if not ret:
            print('The End')
            break    
        
        objects         = objAruco.detect_aruco_pose(img)
        img             = objAruco.draw_image(img, objAruco.corners, objAruco.ids)
        
        cv2.imshow(namew,img)
        ch = cv2.waitKey(10) & 0xFF
        if ch == ord('q'):
            break 

    cv2.destroyAllWindows() 
    print('Video done')       
     
# -------------------------- 
if __name__ == '__main__':
    print(__doc__)

    #test_image()
    test_camera() # ok
    #test_video()
    
    
    
    