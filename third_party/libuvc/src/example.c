#include "libuvc/libuvc.h"
#include <stdio.h>
#include <cv.h>
#include <highgui.h>

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb_rgb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;

  /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    printf("unable to allocate bgr frame!");
    return;
  }

  /* Do the BGR conversion */
  ret = uvc_any2bgr(frame, bgr);
  if (ret) {
    uvc_perror(ret, "uvc_any2bgr");
    uvc_free_frame(bgr);
    return;
  }

  /* Call a user function:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_user_function(ptr, bgr);
   * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
   */

  /* Call a C++ method:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_obj->my_func(bgr);
   */

  /* Use opencv.highgui to display the image: */
  IplImage* cvImg;
    cvImg = cvCreateImageHeader(
        cvSize(bgr->width, bgr->height),
        IPL_DEPTH_8U,
        3);
   
    cvSetData(cvImg, bgr->data, bgr->width * 3); 
   
    cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
    cvShowImage("Test", cvImg);
    cvWaitKey(10);
   
    cvReleaseImageHeader(&cvImg);
   

  uvc_free_frame(bgr);
}

/*Convert a YUYV -advertised image to a gray16 image for OpenCV */

uvc_error_t uvc_yuyv2gray16(uvc_frame_t *in) {
  if (in->frame_format != UVC_FRAME_FORMAT_YUYV)
    return UVC_ERROR_INVALID_PARAM;

  in->frame_format = UVC_FRAME_FORMAT_GRAY16;
  return UVC_SUCCESS;
}


//Callback for the 16-bit gray depthmap
void cb_gray(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *gray;
  uvc_error_t ret;

  gray = frame;

  gray->frame_format = UVC_FRAME_FORMAT_GRAY16;

  /* Use opencv.highgui to display the image: */
  IplImage* cvImg;
    cvImg = cvCreateImageHeader(
        cvSize(gray->width, gray->height),
        IPL_DEPTH_16U,
        1);
   
    cvSetData(cvImg, gray->data, gray->width * 2); 
   
    cvNamedWindow("Test Depth", CV_WINDOW_AUTOSIZE);
    cvShowImage("Test Depth", cvImg);
    cvWaitKey(10);
   
    cvReleaseImageHeader(&cvImg);
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh_d;
  uvc_device_handle_t *devh_rgb;
  uvc_stream_ctrl_t ctrl_rgb;
  uvc_stream_ctrl_t ctrl_d;
 
  uvc_error_t res;

  /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");


  /* Locates the first attached UVC device, stores in dev */
  res = uvc_find_device(
      ctx, &dev,
      0x8086, 0x0a66, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); /* no devices found */
  } else {
    puts("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open2(dev, &devh_rgb, 0); //Try to open camera 0 (RGB)
    

    if (res < 0) {
      uvc_perror(res, "uvc_open"); /* unable to open device */
    } else {
      puts("RGB Device opened");

      /* Print out a message containing all the information that libuvc
       * knows about the device */
      uvc_print_diag(devh_rgb, stderr);

      
      printf("Opening depth camera\n");
      res = uvc_open2(dev, &devh_d, 1); //Try to open camera 1  (RGB)
      uvc_print_diag(devh_d, stderr);
      
      
      /* Try to negotiate a 640x480 30 fps YUYV stream profile */
      res = uvc_get_stream_ctrl_format_size(
          devh_rgb, &ctrl_rgb, /* result stored in ctrl */
          UVC_FRAME_FORMAT_YUYV, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
	  640, 480, 30 /* width, height, fps */
      );
      /* Print out the result */
      uvc_print_stream_ctrl(&ctrl_rgb, stderr);

      /* Try to negotiate a 640x480 30 fps YUYV stream profile */
      res = uvc_get_stream_ctrl_format_size(
          devh_d, &ctrl_d, /* result stored in ctrl */
          UVC_FRAME_FORMAT_YUYV, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
          640, 480, 30 /* width, height, fps */
      );

      /* Print out the result */
      uvc_print_stream_ctrl(&ctrl_d, stderr);

      if (res < 0) {
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
      } else {
        /* Start the video stream. The library will call user function cb:
         *   cb(frame, (void*) 12345)
         */
	

        res = uvc_start_streaming(devh_rgb, &ctrl_rgb, cb_rgb, 12345, 0);


	res = uvc_start_streaming(devh_d, &ctrl_d, cb_gray, 12345, 0);

        if (res < 0) {
          uvc_perror(res, "start_streaming"); /* unable to start stream */
        } else {
          puts("Streaming...");

          //uvc_set_ae_mode(devh, 1); /* e.g., turn on auto exposure */

          sleep(30); /* stream for 10 seconds */

          /* End the stream. Blocks until last callback is serviced */
          uvc_stop_streaming(devh_rgb);
	  uvc_stop_streaming(devh_d);

          puts("Done streaming.");
        }
      }

      /* Release our handle on the device */
      uvc_close(devh_d);
      uvc_close(devh_rgb);

      puts("Device closed");
    }

    /* Release the device descriptor */
    uvc_unref_device(dev);
  }

  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  puts("UVC exited");

  return 0;
}

