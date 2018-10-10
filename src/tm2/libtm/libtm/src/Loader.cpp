/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include <iostream>
#include </usr/include/libusb-1.0/libusb.h>
#include "Loader.h"

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>


using namespace std;
using namespace tracking;



static inline void highres_gettime(highres_time_t *ptr) {
    clock_gettime(CLOCK_REALTIME, ptr);
}

static inline double highres_elapsed_ms(highres_time_t *start, highres_time_t *end) {
    struct timespec temp;
    if((end->tv_nsec - start->tv_nsec) < 0) {
        temp.tv_sec = end->tv_sec - start->tv_sec - 1;
        temp.tv_nsec = 1000000000 + end->tv_nsec-start->tv_nsec;
    } else {
        temp.tv_sec = end->tv_sec - start->tv_sec;
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return (double)(temp.tv_sec * 1000) + (((double)temp.tv_nsec) * 0.000001);
}


Status tracking::loader::send_file(const char *fname) {
    FILE *file;
    uint8_t *tx_buf, *p;
    int res;
    int wb, twb, wbr;
    unsigned long filesize;
    double elapsedTime;
    highres_time_t t1, t2;

    file = fopen(fname, "rb");
    if(file == NULL) {
        perror(fname);
        return FILE_IO_ERROR;
    }

    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);

    if(!(tx_buf = (uint8_t*)malloc(filesize))) {
        perror("buffer");
        fclose(file);
        return INTERNAL_ERROR;
    }

    if(fread(tx_buf, 1, filesize, file) != filesize) {
        perror(fname);
        fclose(file);
        free(tx_buf);
        return FILE_IO_ERROR;
    }
    fclose(file);

    elapsedTime = 0;
    twb = 0;
    p = tx_buf;

    printf("Performing bulk write of %lu byte%s from %s...\n", filesize, filesize == 1 ? "" : "s", fname);

    while(twb < filesize) {
        highres_gettime(&t1);
        wb = filesize - twb;
        if(wb > DEFAULT_CHUNKSZ)
            wb = DEFAULT_CHUNKSZ;
        wbr = 0;
        res = libusb_bulk_transfer(m_dev_handle, EP_OUT, p, wb, &wbr, TIMEOUT_MS);
        if((res == USB_ERR_TIMEOUT) && (twb != 0))
            break;
        highres_gettime(&t2);
        if((res < 0) || (wb != wbr)) {
            fprintf(stdout, "bulk write: %d\n", res);
            free(tx_buf);
            return USB_OPERATION_FAILED;
        }
        elapsedTime += highres_elapsed_ms(&t1, &t2);
        twb += wbr;
        p += wbr;
    }

    double MBpS =
            ((double)filesize / 1048576.) /
            (elapsedTime * 0.001);

    printf("Successfully sent %ld bytes of data in %lf ms (%lf MB/s)\n", filesize, elapsedTime, MBpS);

    free(tx_buf);

    return SUCCESS;
}

Status tracking::loader::load_image(char *filename)
{
    libusb_context *context = NULL;

    Status ret_val = SUCCESS;

    int st = libusb_init(&context);

    if (st!=0)
    {
        printf("Error while initializing libusb\n");
        return USB_INIT_ERROR;
    }

    m_dev_handle = libusb_open_device_with_vid_pid(context, m_vid, m_pid);

    if (m_dev_handle == NULL) {
        return INTERNAL_ERROR;
    }

    ret_val = send_file(filename);

    libusb_close(m_dev_handle);

    return ret_val;
}

