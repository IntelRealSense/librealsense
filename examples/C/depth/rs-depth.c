// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_frame.h>
#include "../example.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

////////////////////////////////////////////
// This parameters are reconfigurable     //
////////////////////////////////////////////
#define STREAM          RS2_STREAM_DEPTH  //
#define FORMAT          RS2_FORMAT_Z16    //
#define WIDTH           1280              //
#define HEIGHT          720               //
#define FPS             30                //
#define STREAM_INDEX    0                 //
////////////////////////////////////////////


#define HEIGHT_RATIO  20
#define WIDTH_RATIO   10
#define ROWS          (HEIGHT / HEIGHT_RATIO) // Each row represented by 20 rows in the original frame
#define ROW_LENGTH    (WIDTH / WIDTH_RATIO) // Each column represented by 10 columns in the original frame
#define DISPLAY_SIZE  ((ROWS + 1) * (ROW_LENGTH + 1))
#define BUFFER_SIZE   (DISPLAY_SIZE * sizeof(char))


float get_depth_unit_value(const rs2_device* const dev)
{
    rs2_error* e = 0;
    rs2_sensor_list* sensor_list = rs2_query_sensors(dev, &e);
    check_error(e);

    int num_of_sensors = rs2_get_sensors_count(sensor_list, &e);
    check_error(e);

    float depth_scale = 0;
    int is_depth_sensor_found = 0;
    int i;
    for (i = 0; i < num_of_sensors; ++i)
    {
        rs2_sensor* sensor = rs2_create_sensor(sensor_list, i, &e);
        check_error(e);

        is_depth_sensor_found = rs2_is_sensor_extendable_to(sensor, RS2_EXTENSION_DEPTH_SENSOR, &e);
        check_error(e);

        if (1 == is_depth_sensor_found)
        {
            depth_scale = rs2_get_option(sensor, RS2_OPTION_DEPTH_UNITS, &e);
            check_error(e);
            rs2_delete_sensor(sensor);
            break;
        }
        rs2_delete_sensor(sensor);
    }
    rs2_delete_sensor_list(sensor_list);

    if (0 == is_depth_sensor_found)
    {
        printf("Depth sensor not found!\n");
        return EXIT_FAILURE;
    }

    return depth_scale;
}


int main()
{
    rs2_error* e = 0;
    /* Create a context object. This object owns the handles to all connected realsense devices. */
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return EXIT_FAILURE;

    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    print_device_info(dev);

    /* Determine depth value corresponding to one meter */
    uint16_t one_meter = (uint16_t)(1.0f / get_depth_unit_value(dev));

    // Create a pipeline to configure, start and stop camera streaming
    rs2_pipeline* pipeline =  rs2_create_pipeline(ctx, &e);
    check_error(e);

    // Create a config instance
    rs2_config* config = rs2_create_config(&e);
    check_error(e);

    // Request a specific configuration
    rs2_config_enable_stream(config, STREAM, STREAM_INDEX, WIDTH, HEIGHT, FORMAT, FPS, &e);
    check_error(e);

    // Check if current device supports depth streaming
    int supports_depth = rs2_config_can_resolve(config, pipeline, &e);
    check_error(e);
    if (0 == supports_depth)
    {
        printf("The connected device doesn't support depth streaming!\n");
        exit(EXIT_FAILURE);
    }

    // Start the pipeline streaming
    rs2_pipeline_profile* pipeline_profile = rs2_pipeline_start_with_config(pipeline, config, &e);
    check_error(e);

    char buffer[BUFFER_SIZE];
    char* out = NULL;
    while (1)
    {
        /* This call waits until a new coherent set of frames is available on a device */
        rs2_frame* frames = rs2_pipeline_wait_for_frames(pipeline, 5000, &e);
        check_error(e);

        int num_of_frames = rs2_embedded_frames_count(frames, &e);
        check_error(e);

        int i;
        for (i = 0; i < num_of_frames; ++i)
        {
            rs2_frame* frame = rs2_extract_frame(frames, i, &e);
            check_error(e);

            /* Retrieve depth data, configured as 16-bit depth values */
            const uint16_t* depth_frame_data = (const uint16_t*)(rs2_get_frame_data(frame, &e));
            check_error(e);

            /* Print a simple text-based representation of the image, by breaking it into 10x5 pixel regions and approximating the coverage of pixels within one meter */
            out = buffer;
            int coverage[ROW_LENGTH] = { 0 }, x, y, i;
            for (y = 0; y < HEIGHT; ++y)
            {
                for (x = 0; x < WIDTH; ++x)
                {
                    // Create a depth histogram to each row
                    int coverage_index = x / WIDTH_RATIO;
                    int depth = *depth_frame_data++;
                    if (depth > 0 && depth < one_meter)
                        ++coverage[coverage_index];
                }

                if ((y % HEIGHT_RATIO) == (HEIGHT_RATIO-1))
                {
                    for (i = 0; i < (ROW_LENGTH); ++i)
                    {
                        const char* pixels = " .:nhBXWW";
                        int pixel_index = (coverage[i] / (HEIGHT_RATIO * WIDTH_RATIO / sizeof(pixels)));
                        *out++ = pixels[pixel_index];
                        coverage[i] = 0;
                    }
                    *out++ = '\n';
                }
            }
            *out++ = 0;
            printf("\n%s", buffer);

            rs2_release_frame(frame);
        }

        rs2_release_frame(frames);
    }

    rs2_pipeline_stop(pipeline, &e);
    check_error(e);

    // Release resources
    rs2_delete_pipeline_profile(pipeline_profile);
    rs2_delete_config(config);
    rs2_delete_pipeline(pipeline);
    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);

    return EXIT_SUCCESS;
}
