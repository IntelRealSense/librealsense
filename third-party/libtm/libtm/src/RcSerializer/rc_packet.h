// Copyright (c) 2008-2012, Eagle Jones
// Copyright (c) 2012-2015 RealityCap, Inc
// All rights reserved.
//

#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef MYRIAD2
// lives in target/host common area
#ifdef USE_TM2_PACKETS
#include "get_pose.h"
#endif
#endif

#ifdef WIN32
#pragma warning (push)
#pragma warning (disable : 4200)
#endif

//WARNING: Do not change the order of this enum, or insert new packet types in the middle
//Only append new packet types after the last one previously defined.
typedef enum packet_type {
    packet_none = 0,
    packet_camera = 1,
    packet_imu = 2,
    packet_feature_track = 3,
    packet_feature_select = 4,
    packet_navsol = 5,
    packet_feature_status = 6,
    packet_filter_position = 7,
    packet_filter_reconstruction = 8,
    packet_feature_drop = 9,
    packet_sift = 10,
    packet_plot_info = 11,
    packet_plot = 12,
    packet_plot_drop = 13,
    packet_recognition_group = 14,
    packet_recognition_feature = 15,
    packet_recognition_descriptor = 16,
    packet_map_edge = 17,
    packet_filter_current = 18,
    packet_feature_prediction_variance = 19,
    packet_accelerometer = 20,
    packet_gyroscope = 21,
    packet_filter_feature_id_visible = 22,
    packet_filter_feature_id_association = 23,
    packet_feature_intensity = 24,
    packet_filter_control = 25,
    packet_ground_truth = 26,
    packet_core_motion = 27,
    packet_image_with_depth = 28,
    packet_image_raw = 29,
    packet_diff_velocimeter = 30,
    packet_thermometer = 31,
    packet_stereo_raw = 40,
    packet_image_stereo = 41,
    packet_rc_pose = 42,
    packet_calibration_json = 43,
    packet_arrival_time = 44,
    packet_velocimeter = 45,
    packet_pose = 46,
    packet_control = 47,
    packet_calibration_bin = 48,
    packet_exposure = 49,
    packet_controller_physical_info = 50,
    packet_led_intensity = 51,
    packet_command_start = 100,
    packet_command_stop = 101,
} packet_type;

typedef struct packet_header_t {
    uint32_t bytes; //size of packet including header
    uint16_t type;  //id of packet
    uint16_t sensor_id;  //id of sensor
    uint64_t time;  //time in microseconds
} packet_header_t;

typedef struct packet_t {
    packet_header_t header;
    uint8_t data[];
} packet_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_camera_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height;
    uint16_t depth_width, depth_height;
    uint8_t data[];
} packet_image_with_depth_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height, stride;
    uint16_t format; // enum { Y8, Z16_mm };
    uint8_t data[];
} packet_image_raw_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height, stride1, stride2;
    uint16_t format; // enum { Y8, Z16_mm };
    uint8_t data[]; // image2 starts at data + height*stride1
} packet_stereo_raw_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    float gain;
} packet_exposure_t;

typedef struct {
    packet_header_t header;
    packet_image_raw_t *frames[2];
} packet_image_stereo_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
    float w[3]; // rad/s
} packet_imu_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
} packet_accelerometer_t;

typedef struct {
    packet_header_t header;
    float w[3]; // rad/s
} packet_gyroscope_t;

typedef struct {
    packet_header_t header;
    float temperature_C;
} packet_thermometer_t;

typedef struct {
    packet_header_t header;
    float v[3];
} packet_velocimeter_t;

typedef struct {
    packet_header_t header;
    float v[2]; // m/s in body x for sensor_id, sensor_id+1
} packet_diff_velocimeter_t;

typedef struct {
    packet_header_t header;
    float rotation_rate[3]; // rad/s
    float gravity[3]; //m/s^2
} packet_core_motion_t;

typedef struct {
    packet_header_t header;
    struct feature_t { float x, y; } features[];
} packet_feature_track_t;

typedef struct {
    packet_header_t header;
    uint16_t indices[];
} packet_feature_drop_t;

typedef struct {
    packet_header_t header;
    uint8_t status[];
} packet_feature_status_t;

typedef struct {
    packet_header_t header;
    uint32_t led_id;
    uint32_t intensity;
}packet_led_intensity_t;

typedef struct {
    packet_header_t header;
    uint8_t intensity[];
} packet_feature_intensity_t;

typedef struct {
    packet_header_t header;
    float latitude, longitude;
    float altitude;
    float velocity[3];
    float orientation[3];
} packet_navsol_t;

typedef struct {
    packet_header_t header;
    float position[3];
    float orientation[3];
} packet_filter_position_t;

typedef struct {
    packet_header_t header;
    float points[][3];
} packet_filter_reconstruction_t;

typedef struct {
    packet_header_t header;
    uint64_t reference;
    float T[3];
    float W[3];
    float points[][3];
} packet_filter_current_t;

typedef struct {
    packet_header_t header;
    float T[3];
    float W[3];
    uint64_t feature_id[];
} packet_filter_feature_id_visible_t;

typedef struct {
    packet_header_t header;
    uint64_t feature_id[];
} packet_filter_feature_id_association_t;

typedef struct packet_listener {
    packet_t *latest;
    int type;
    bool isnew;
} packet_listener_t;

typedef struct {
    packet_header_t header;
    float nominal;
    const char identity[];
} packet_plot_info_t;

typedef struct {
    packet_header_t header;
    int count;
    float data[];
} packet_plot_t;

typedef struct {
    packet_header_t header;
    uint64_t first, second;
    float T[3], W[3];
    float T_var[3], W_var[3];
} packet_map_edge_t;

typedef struct {
    packet_header_t header;
    int64_t id; //signed to indicate add/drop
    float W[3], W_var[3];
} packet_recognition_group_t;

typedef struct {
    packet_header_t header;
    uint64_t groupid;
    uint64_t id;
    float ix;
    float iy;
    float depth;
    float x, y, z;
    float variance;
} packet_recognition_feature_t;

typedef struct {
    packet_header_t header;
    struct feature_covariance_t { float x, y, cx, cy, cxy; } covariance[];
} packet_feature_prediction_variance_t;

typedef struct {
    packet_header_t header;
    uint64_t groupid;
    uint32_t id;
    float color;
    float x, y, z;
    float variance;
    uint32_t label;
    uint8_t descriptor[128];
} packet_recognition_descriptor_t;

typedef struct {
    packet_header_t header;
    float T[3];
    float velocity[3]; // m/s
    float rotation[4]; // axis [0:2] angle in rad [3]
    float w[3]; // rad/s
    float w_a[3]; // rad/s^2
} packet_ground_truth_t;

enum packet_plot_type {
    packet_plot_var_T,
    packet_plot_var_W,
    packet_plot_var_a,
    packet_plot_var_w,
    packet_plot_inn_a,
    packet_plot_inn_w,
    packet_plot_meas_a,
    packet_plot_meas_w,
    packet_plot_unknown = 256
};

#ifdef MYRIAD2
#ifdef USE_TM2_PACKETS
typedef struct {
    packet_header_t header;
    sixDof_data data;
} packet_pose_t;
#endif
#endif

typedef struct {
    packet_header_t header;
    char data[];
} packet_calibration_json_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_calibration_bin_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_controller_physical_info_t;

typedef struct {
    packet_header_t header; // header::timestamp is packet arrival time (To algo), header::sensor_id is not used
} packet_arrival_time_t;

typedef struct packet_control_t {
    struct {
        uint32_t bytes; //size of packet including header
        uint16_t type;  //id of packet
        union {
            uint16_t sensor_id;  //id of sensor
            uint16_t control_type;  //id of control packet
        };
        uint64_t time;  //time in microseconds
    } header;
    uint8_t data[];
} packet_control_t;

#ifdef WIN32
#pragma warning (pop)
#endif

#endif