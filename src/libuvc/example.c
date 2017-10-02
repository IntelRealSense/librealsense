#include "libuvc.h"
#include <stdio.h>

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb(uvc_frame_t *frame, void *ptr) {
  puts("frame");
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_stream_ctrl_t ctrl;
  uvc_error_t res;
  uvc_format_t *formats;
  uvc_stream_info_t *streams;
  uvc_stream_info_t *cur_stream;
  uvc_format_t *cur_format;
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
      0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); /* no devices found */
  } else {
    puts("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open2(dev, &devh,0);
    uvc_close(devh);
    res = uvc_open2(dev, &devh,0);

    if (res < 0) {
      uvc_perror(res, "uvc_open"); /* unable to open device */
    } else {
      puts("Device opened");

      /* Print out a message containing all the information that libuvc
       * knows about the device */
      //uvc_print_diag(devh, stderr);

      uvc_get_available_streams(devh, &streams);
      cur_stream = streams;

      while (cur_stream != NULL) {

        printf("Stream idx : %d\n",cur_stream->interface_idx);
        uvc_get_available_formats(devh, cur_stream->interface_idx, &formats);
        cur_format = formats;
        while(cur_format != NULL) {
          printf("   Format %08X : w:%03d h:%03d fps:%d\n",
                 cur_format->fourcc,
                 cur_format->width,
                 cur_format->height,
                 cur_format->fps);

          cur_format = cur_format->next;
        }
        uvc_free_formats(formats);
        cur_stream = cur_stream->next;
      }
      uvc_free_streams(streams);

        uvc_get_available_formats_all(devh, &formats);
      /* Try to negotiate a 640x480 30 fps YUYV stream profile */
      res = uvc_get_stream_ctrl_format_size_all(
          devh, &ctrl, /* result stored in ctrl */
          1496860960, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
          640,480,30
      );

      /* Print out the result */
      uvc_print_stream_ctrl(&ctrl, stderr);

      if (res < 0) {
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
      } else {
        /* Start the video stream. The library will call user function cb:
         *   cb(frame, (void*) 12345)
         */
        puts("start");
	    res = uvc_start_streaming(devh, &ctrl, cb, 12345, 0);

        if (res < 0) {
          uvc_perror(res, "start_streaming"); /* unable to start stream */
        } else {
          puts("Streaming...");

          uvc_set_ae_mode(devh, 1); /* e.g., turn on auto exposure */

          sleep(10); /* stream for 10 seconds */

          /* End the stream. Blocks until last callback is serviced */
          uvc_stop_streaming(devh);
          puts("Done streaming.");
        }
      }

      /* Release our handle on the device */
      uvc_close(devh);
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

