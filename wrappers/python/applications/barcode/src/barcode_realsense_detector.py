

"""
Created on Sep 04 16:53:13 2019
Debug module that simulates detection

-----------------------------
 Ver    Date     Who    Descr
-----------------------------
0101   01.07.24  UD     Created 
-----------------------------
"""

import cv2
import numpy as np
import copy
try:
    from pyzbar.pyzbar import decode , ZBarSymbol
except:
    print('Warning : Barcode lib is not found on not installed.')
    decode , ZBarSymbol = None, None

# ----------------------------------------
class ObjectAbstract:
    """
    standard abstract type for the RealSense object
    mostly defines output and data representation
    """
    def __init__(self):
            
        self.name          = 'general'
        self.id            = 1  # many different object
        self.pose          = np.zeros((6,1))  # Tx,Ty,Tz in mm and Rx, Ry Rz in degrees
        self.quality       = 0  # quality between 0-1

    def get_object_data(self):
        "read object data"
        object_data        = {'name':self.name, 'id':self.id, 'pose':self.pose,'quality':self.quality}
        return object_data



# ----------------------------------------
class ObjectBarcode(ObjectAbstract):

    """ 
    Creates and manage barcode detector

    """

    def __init__(self):
            
        self.name          = 'barcode'
        self.code           = [] # barcode array
        self.no_library     = decode is None
        self.debugOn        = False

    
    def detect_objects_old(self, rgb_image, objects = None):
        
        """
        detect multiple barcodes - select one which is in the middle of the page
        
        """
        
        # select object to work with
        objects = {'objectId': [], 'rvecAll': [], 'tvecAll': [], 'objectQ': [], 'objectText': []}
        if self.no_library:
            self.Print("Failed to find barcode library",'E' )
            return objects
            
        img_x_center                = rgb_image.shape[1]/2
         
        code, rgb_image             = self.detect_barcode(rgb_image)
        self.code                   = code
        barcode_num                 = len(code)
        if barcode_num < 1:
            #self.Print("Failed to find barcode" , 'W')
            return objects
        
        # compute positions in x of the barcodes
        x_coord                     = [code[k].rect[0] for k in range(barcode_num)]
        x_coord                     = np.array(x_coord)
        
        cent_ind                        = np.argmin(np.abs(x_coord - img_x_center))
        
        # select the central only
        barcode_txt                    = code[cent_ind].data.decode ('utf-8')
        
        # compute pose in x and y
        (x, y, w, h)                   = code[cent_ind].rect
        
        # save to output
        objects['objectId']            = [barcode_txt]
        objects['rvecAll']             = [np.zeros((3,1))]
        objects['tvecAll']             = [np.array([x,y,0]).reshape((3,1))]
        objects['objectQ']             = [1]  # reliable
        objects['objectText']          = ['barcode']  # reliable
                
        return objects        
    
    def detect_objects(self, rgb_image):
        """
        detect multiple barcodes
        
        """

        # select object to work with
        objects = {'objectId': [], 'rvecAll': [], 'tvecAll': [], 'objectQ': [], 'objectText': []}
        if self.no_library:
            self.Print("Failed to find barcode library",'E' )
            return objects        
        
        code, rgb_image             = self.detect_barcode(rgb_image)
        self.code                   = code
        if len(code)<1:
            #self.Print("Failed to find barcode" , 'W')
            return objects
        
        barcode_txt                    = code[0].data.decode ('utf-8')
        
        # compute pose in x and y
        (x, y, w, h)                   = code[0].rect
        
         
        #rvec, tvec          = self.get_object_pose(self.pattern_points, corners, self.camera_matrix, self.dist_coeffs)        
      
        objects['objectId']            = [barcode_txt]
        objects['rvecAll']             = [np.zeros((3,1))]
        objects['tvecAll']             = [np.array([x,y,0]).reshape((3,1))]
        objects['objectQ']             = [1]  # reliable
        objects['objectText']          = ['barcode']  # reliable
                
        return objects
    

    
    def detect_barcode(self, frame_rgb, objectsInfo = None):
        # if detcted the region
        
        # use detcetion from nnet to extract the sub region
        self.region_start_x = 0
        self.region_start_y = 0
        if objectsInfo is not None:
            if len(objectsInfo['objectPoints']) > 0:
                # xy array
                object_points     = objectsInfo['objectPoints'][0]
                min_vals          = object_points.min(axis = 0)
                max_vals          = object_points.max(axis = 0)
                min_x             = np.maximum(0, min_vals[0] - 5).astype(int)
                max_x             = np.minimum(frame_rgb.shape[1], max_vals[0] + 5).astype(int)
                min_y             = np.maximum(0, min_vals[1] - 15).astype(int)
                max_y             = np.minimum(frame_rgb.shape[0], max_vals[1] + 15).astype(int)            
                frame_rgb         = frame_rgb[min_y:max_y,min_x:max_x,:]  
                
                self.region_start_x = min_x
                self.region_start_y = min_y

        
        img     = frame_rgb

        # read the image in numpy array using cv2
        #img         = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        #blur        = cv2.GaussianBlur(img, (5, 5), 0) 
        #ret, bw_im = cv2.threshold(blur, 0, 255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
        #cv2.imshow('Barcode',img)
        #cv2.waitKey(10)
        
        #tstart      = time.time()
        rotation_angles_degree = [0]
        for k in range(len(rotation_angles_degree)):
            #rstart      = time.time()
            img_rot     = self.rotate_image(img, rotation_angles_degree[k])
            #self.Print('Rotation time : %4.3f' %(time.time() - rstart))
            code        = decode(img_rot) #, symbols=[ZBarSymbol.CODE128])
            #self.Print('Detection time : %4.3f' %(time.time() - rstart))
            if len(code) > 0:
                break
        #self.Print('Total Detection time : %4.3f' %(time.time() - tstart))    
        # debug
        #print (code)
        #self.draw_image(img_rot, code)

        return code, img    

    def draw_image(self, color_image, barcode = []):
        # show image and barcode
        color_image = self.draw_barcode(color_image, 1, barcode)
        cv2.imshow('Barcode Image',color_image)
        cv2.waitKey(100)

        return color_image
    
    def rotate_image(self, img, rot_ang):
        # rotate image by degrees to simplify detection
        h, w        = img.shape[0], img.shape[1]
        degree      = rot_ang
        R           = cv2.getRotationMatrix2D((w / 2, h / 2), degree, 1)
        img_rot     = copy.deepcopy(img)
        img_rot     = cv2.warpAffine(img_rot, R, (w, h), flags=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT, borderValue=0)
        
        return img_rot    
    
    def draw_barcode(self, color_image, scaleFactor = 1, detectedBarcodes = None):
        
        detectedBarcodes = self.code if detectedBarcodes is None else detectedBarcodes
        
        # Traverse through all the detected barcodes in image
        for barcode in detectedBarcodes:
        
            # Locate the barcode position in image
            (x, y, w, h) = barcode.rect 
            # correct for subregion
            x, y, w, h   = int(x * scaleFactor), int(y * scaleFactor), int(w * scaleFactor), int(h * scaleFactor)
            
            # Put the rectangle in image using
            # cv2 to highlight the barcode
            cv2.rectangle(color_image, (x-10, y-10), (x + w+10, y + h+10),  (0, 255, 255), 2)
            cv2.putText(color_image, (barcode.data.decode ('utf-8')), (x-100, y-20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA, False)
            
            if barcode.data !="" and self.debugOn:
            
                # Print the barcode data
                print(barcode.data.decode ('utf-8'))
                print(barcode.type)
                print(barcode.rect)
              
        # support resize
        return color_image
    
    def Print(self, txt='',level='I'):
        
        if level == 'I':
            ptxt = 'I: BD: %s' % txt
            #logging.info(ptxt)  
        if level == 'W':
            ptxt = 'W: BD: %s' % txt
            #logging.warning(ptxt)  
        if level == 'E':
            ptxt = 'E: BD: %s' % txt
            #logging.error(ptxt)  
           
        print(ptxt)
    
# ----------------------------------------        
# Tests
# ----------------------------------------

def test_image():
    "read barcodes from images"
    import glob
    barcode_list         = []
    img_list            = glob.glob(r'..\data\*.jpg')
    oBarCode            = ObjectBarcode()
    
    for i, img_name in enumerate(img_list):
        img            = cv2.imread(img_name)
        objects        = oBarCode.pose6d_detect_objects(img)
#        code           = 
#        barcode_list.append(objects)
#        if len(code)<1:
#            raise Exception("Failed to find corners in img # %d" % i)
        img             = oBarCode.draw_barcode(img )
        oBarCode.draw_image(img)
        
        cv2.imshow('bars',img)        
        ch = cv2.waitKey() & 0xFF
        if ch == ord('q'):
            break 
        
def test_camera():
    cap                 = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)                     

    oBarCode            = ObjectBarcode()
    
    namew = 'Barcode Demo : q - to quit'
    cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 
    
    while(cap.isOpened()):
        
        ret, img = cap.read()
        if not ret:
            print('The End')
            break    

        objects        = oBarCode.pose6d_detect_objects(img)
        img            = oBarCode.draw_barcode(img )
        #oBarCode.draw_image(img) 
        
        cv2.imshow(namew,img)
        ch = cv2.waitKey(10) & 0xFF
        if ch == ord('q'):
            break 
        
def test_video():

    fname               = r'file:///D:/Users/zion/Downloads/fixture_aruco_test (1).avi'
    cap                 = cv2.VideoCapture(fname)
    oBarCode            = ObjectBarcode()
    
    namew = 'Barcode Demo : q - to quit'
    cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 
    
    while(cap.isOpened()):
        
        ret, img = cap.read()
        if not ret:
            print('The End')
            break    
                 
        objects         = oBarCode.pose6d_detect_objects(img)
        img             = oBarCode.draw_barcode(img )
        #oBarCode.draw_image(img)  
        
        cv2.imshow(namew,img)
        ch = cv2.waitKey(10) & 0xFF
        if ch == ord('q'):
            break 

    cv2.destroyAllWindows() 
    print('Video done')   

def test_camera_realsense():
    """
    Real Sense camera connect
    """
    import pyrealsense2 as rs

    # Configure depth and color streams
    pipeline = rs.pipeline()
    config = rs.config()

    # Get device product line for setting a supporting resolution
    pipeline_wrapper = rs.pipeline_wrapper(pipeline)
    pipeline_profile = config.resolve(pipeline_wrapper)
    device = pipeline_profile.get_device()
    device_product_line = str(device.get_info(rs.camera_info.product_line))

    found_rgb = False
    for s in device.sensors:
        if s.get_info(rs.camera_info.name) == 'RGB Camera':
            found_rgb = True
            break
    if not found_rgb:
        print("The demo requires Depth camera with Color sensor")
        exit(0)

    #config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
    config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 30)

    # Start streaming
    pipeline.start(config)

    # barcode
    oBarCode            = ObjectBarcode()

    namew = 'RealSense Barcode Demo : q - to quit'
    cv2.namedWindow(namew,cv2.WINDOW_AUTOSIZE) 

    try:
        while True:

            # Wait for a coherent pair of frames: depth and color
            frames = pipeline.wait_for_frames()
            depth_frame = True #frames.get_depth_frame()
            color_frame = frames.get_color_frame()
            if not depth_frame or not color_frame:
                continue

            # Convert images to numpy arrays
            #depth_image = np.asanyarray(depth_frame.get_data())
            color_image = np.asanyarray(color_frame.get_data())

            # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
            #depth_colormap = cv2.applyColorMap(cv2.convertScaleAbs(depth_image, alpha=0.03), cv2.COLORMAP_JET)

            #depth_colormap_dim = depth_colormap.shape
            color_colormap_dim = color_image.shape

            # If depth and color resolutions are different, resize color image to match depth image for display
            # if depth_colormap_dim != color_colormap_dim:
            #     resized_color_image = cv2.resize(color_image, dsize=(depth_colormap_dim[1], depth_colormap_dim[0]), interpolation=cv2.INTER_AREA)
            #     images = np.hstack((resized_color_image, depth_colormap))
            # else:
            #     images = np.hstack((color_image, depth_colormap))
            images          = color_image

            # Show images
            #cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
            #cv2.imshow('RealSense', images)
            #cv2.waitKey(1)

            objects         = oBarCode.detect_objects(images)
            img             = oBarCode.draw_barcode(images)
             
        
            cv2.imshow(namew,img)
            ch = cv2.waitKey(10) & 0xFF
            if ch == ord('q'):
                break 

    finally:

        # Stop streaming
        pipeline.stop()
    
     
# -------------------------- 
if __name__ == '__main__':
    print(__doc__)

    #test_camera() # ok
    #test_video()
    #test_image()
    test_camera_realsense()
