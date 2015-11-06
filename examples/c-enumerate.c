#include <librealsense/rs.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void check_error(rs_error * error);

int main()
{
    rs_error * e = 0;
    rs_context * context;
    rs_device * device;
    rs_option option;
    rs_stream stream;
    rs_format format;
    rs_intrinsics intrin;
    int i, j, k, device_count, mode_count;
    int width, height, framerate;
    float hfov, vfov;

    /* Obtain a list of devices currently present on the system */
    context = rs_create_context(RS_API_VERSION, &e);
    check_error(e);

    device_count = rs_get_device_count(context, &e);
    check_error(e);
    
    if (!device_count) printf("No device detected. Is it plugged in?\n");

    for(i = 0; i < device_count; ++i)
    {
        /* Show the device name */
        device = rs_get_device(context, i, &e);
        check_error(e);

        printf("Device %d - %s:\n", i, rs_get_device_name(device, &e));
        check_error(e);
        printf("  Serial number: %s\n", rs_get_device_serial(device, &e));
        check_error(e);

        /* Show which options are supported by this device */
        printf("  Supported options:\n");
        for(j = 0; j < RS_OPTION_COUNT; ++j)
        {
            option = (rs_option)j;
            if(rs_device_supports_option(device, option, &e)) printf("    %s\n", rs_option_to_string(option));
            check_error(e);
        }

        /* Show which streams are supported by this device */
        for(j = 0; j < RS_STREAM_COUNT; ++j)
        {
            /* Determine number of available streaming modes (zero means stream is unavailable) */
            stream = (rs_stream)j;
            mode_count = rs_get_stream_mode_count(device, stream, &e);
            check_error(e);
            if(mode_count == 0) continue;

            /* Show each available mode for this stream */
            printf("  Stream %s - %d modes:\n", rs_stream_to_string(stream), mode_count);
            for(k = 0; k < mode_count; ++k)
            {
                /* Show width, height, format, and framerate, the settings required to enable the stream in this mode */
                rs_get_stream_mode(device, stream, k, &width, &height, &format, &framerate, &e);
                check_error(e);

                printf("    %dx%d\t%s\t%dHz", width, height, rs_format_to_string(format), framerate);

                /* Enable the stream in this mode so that we can retrieve its intrinsics */
                rs_enable_stream(device, stream, width, height, format, framerate, &e);
                check_error(e);

                rs_get_stream_intrinsics(device, stream, &intrin, &e);
                check_error(e);

                /* Show horizontal and vertical field of view, in degrees, and whether the mode produced rectilinear or distorted images */
                hfov = (atan2f(intrin.ppx + 0.5f, intrin.fx) + atan2f(width - intrin.ppx - 0.5f, intrin.fx)) * 57.2957795f;
                vfov = (atan2f(intrin.ppy + 0.5f, intrin.fy) + atan2f(height - intrin.ppy - 0.5f, intrin.fy)) * 57.2957795f;
                printf("\t%.1f x %.1f degrees, %s\n", hfov, vfov, intrin.model == RS_DISTORTION_NONE ? "rectilinear" : "distorted");
            }

            /* Some stream mode combinations are invalid, so disable this stream before moving on to the next one */
            rs_disable_stream(device, stream, &e);
        }
    }

    /* Free resources owned by the library (including all rs_device instances) */
    rs_delete_context(context, &e);
    check_error(e);
    return 0;
}

void check_error(rs_error * error)
{
    if (error)
    {
        fprintf(stderr, "error calling %s(%s):\n  %s\n", rs_get_failed_function(error), rs_get_failed_args(error), rs_get_error_message(error));
        rs_free_error(error);
        exit(EXIT_FAILURE);
    }
}
