#include "rs_hid.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, const char * argv[]) {
    int ret;
    rs_hid_device_info *devices = RS_hid_Enumerate(0x8086, 0x0);
    
    if (devices == NULL) {
        printf("can't find any real sense devices\n");
        exit(-1);
    }
    
    hid_device *device = hid_open(devices[0].vendor_id, devices[0].product_id, devices[0].serial_number);
    
    if (device == NULL) {
        printf("fail to open device\n");
        exit(-1);
    }
    
    
    ret = RS_hid_PowerDevice(device, REPORT_ID_ACCELEROMETER_3D);
    if (ret == -1) {
        printf("Fail to power device\n");
        exit(-1);
    }
    ret = RS_hid_PowerDevice(device, REPORT_ID_GYROMETER_3D);
    if (ret == -1) {
        printf("Fail to power device\n");
        exit(-1);
    }


    unsigned char buffer[1024];
    int bytesRead;
    
    do {
        ret = RS_hid_read(device, buffer, &bytesRead);
        if (ret != -1) {
            if (buffer[0] == REPORT_ID_GYROMETER_3D) {
                struct REALSENSE_GYROMETER_REPORT *report = (struct REALSENSE_GYROMETER_REPORT *)buffer;
                printf("GYRO:TS:%08x X:%04X Y:%04X Z:%04X\n",
                 report->timeStamp,
                 report->velocityX,
                 report->velocityY,
                 report->velocityZ);
            } else if (buffer[0] == REPORT_ID_ACCELEROMETER_3D) {
                struct REALSENSE_ACCELEROMETER_REPORT *report = (struct REALSENSE_ACCELEROMETER_REPORT *)buffer;
                printf("ACCE:TS:%08x X:%04X Y:%04X Z:%04X\n",
                       report->timeStamp,
                       report->accelerationAxisX,
                       report->accelerationAxisY,
                       report->accelerationAxisZ);
                
            }
        } else {
            printf("Fail to read interrupt data.\n");
            break;
        }
    } while(1);
    
    RS_hid_close(device);
    return 0;
}
