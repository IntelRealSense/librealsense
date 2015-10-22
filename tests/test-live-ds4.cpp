#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include <librealsense/rs.h>

#include <thread>

// RAII wrapper to ensure that contexts are always cleaned up. If this is not done, subsequent 
// contexts may not enumerate devices correctly. Long term, we should probably try to remove 
// this requirement, or at least throw an error if the user attempts to open multiple contexts at once.
class safe_context
{
    rs_context * context;
    safe_context(int) : context() {}
public:
    safe_context() : safe_context(1) 
    {
        rs_error * error = nullptr;
        context = rs_create_context(RS_API_VERSION, &error);  
        REQUIRE(error == nullptr);
        REQUIRE(context != nullptr);
    }

    ~safe_context()
    {
        if(context)
        {
            rs_delete_context(context, nullptr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    operator rs_context * () const { return context; }
};

#ifdef WIN32
#define NOEXCEPT_FALSE
#else
#define NOEXCEPT_FALSE noexcept(false)
#endif

class require_error
{
    const char * message;
    rs_error * err;
public:
    require_error(const char * message) : message(message), err() {}
    require_error(const require_error &) = delete;
    ~require_error() NOEXCEPT_FALSE
    {
        if(std::uncaught_exception()) return;
        REQUIRE(err != nullptr);
        REQUIRE(rs_get_error_message(err) == std::string(message));
    }
    require_error &  operator = (const require_error &) = delete;
    operator rs_error ** () { return &err; }
};

class require_no_error
{
    rs_error * err;
public:
    require_no_error() : err() {}
    require_no_error(const require_error &) = delete;
    ~require_no_error() NOEXCEPT_FALSE { if(!std::uncaught_exception()) REQUIRE(err == nullptr); }
    require_no_error &  operator = (const require_no_error &) = delete;
    operator rs_error ** () { return &err; }
};

float dot_product(const float (& a)[3], const float (& b)[3]) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

void require_cross_product(const float (& r)[3], const float (& a)[3], const float (& b)[3])
{
    REQUIRE( r[0] == Approx(a[1]*b[2] - a[2]*b[1]) );
    REQUIRE( r[1] == Approx(a[2]*b[0] - a[0]*b[2]) );
    REQUIRE( r[2] == Approx(a[0]*b[1] - a[1]*b[0]) );
}

void require_rotation_matrix(const float (& matrix)[9])
{
    const float row0[] = {matrix[0], matrix[3], matrix[6]};
    const float row1[] = {matrix[1], matrix[4], matrix[7]};
    const float row2[] = {matrix[2], matrix[5], matrix[8]};
    REQUIRE( dot_product(row0, row0) == Approx(1) );
    REQUIRE( dot_product(row1, row1) == Approx(1) );
    REQUIRE( dot_product(row2, row2) == Approx(1) );
    REQUIRE( dot_product(row0, row1) == Approx(0) );
    REQUIRE( dot_product(row1, row2) == Approx(0) );
    REQUIRE( dot_product(row2, row0) == Approx(0) );
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2); 
}

void require_identity_matrix(const float (& matrix)[9])
{
    static const float identity_matrix_3x3[] = {1,0,0, 0,1,0, 0,0,1};
    for(int i=0; i<9; ++i) REQUIRE( matrix[i] == identity_matrix_3x3[i] );
}

TEST_CASE( "a single DS4 behaves as expected", "[live] [ds4] [one-camera]" )
{
    safe_context ctx;
    
    SECTION( "exactly one device is connected" )
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION( "device name is Intel RealSense R200" )
    {
        const char * name = rs_get_device_name(dev, require_no_error());
        REQUIRE(name == std::string("Intel RealSense R200"));
    }

    SECTION( "device serial number has ten decimal digits" )
    {
        const char * serial = rs_get_device_serial(dev, require_no_error());
        REQUIRE(strlen(serial) == 10);
        for(int i=0; i<10; ++i) REQUIRE(isdigit(serial[i]));
    }

    SECTION( "device firmware version is a nonempty string" )
    {
        const char * version = rs_get_device_firmware_version(dev, require_no_error());
        REQUIRE(strlen(version) > 0);
    }

    SECTION( "device supports standard picture options and R200 extension options, and nothing else" )
    {
        for(int i=0; i<RS_OPTION_COUNT; ++i)
        {
            if(i >= RS_OPTION_COLOR_BACKLIGHT_COMPENSATION && i <= RS_OPTION_COLOR_WHITE_BALANCE)
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
            else if(i >= RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED && i <= RS_OPTION_R200_DISPARITY_SHIFT)
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
            else
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 0);
            }
        }

        // Require the option requests with indices outside of [0,RS_OPTION_COUNT) indicate an error
        rs_device_supports_option(dev, (rs_option)-1, require_error("bad enum value for argument \"option\""));
        rs_device_supports_option(dev, RS_OPTION_COUNT, require_error("bad enum value for argument \"option\""));
    }

    SECTION( "no extrinsic transformation between DEPTH and INFRARED" )
    {
        rs_extrinsics extrin;
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED, &extrin, require_no_error());

        require_identity_matrix(extrin.rotation);
        for(int i=0; i<3; ++i) REQUIRE(extrin.translation[i] == 0.0f);
    }

    SECTION( "only x-axis translation (~70 mm) between DEPTH and INFRARED2" )
    {
        rs_extrinsics extrin;
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED2, &extrin, require_no_error());

        require_identity_matrix(extrin.rotation);
        REQUIRE(extrin.translation[0] < -0.06f); // Some variation is allowed, but should report at least 60 mm in all cases
        REQUIRE(extrin.translation[0] > -0.08f); // Some variation is allowed, but should report at most 80 mm in all cases
        for(int i=1; i<3; ++i) REQUIRE(extrin.translation[i] == 0.0f);
    }

    SECTION( "extrinsics between DEPTH and COLOR contain a valid rotation matrix and translation vector" )
    {
        rs_extrinsics extrin;
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_COLOR, &extrin, require_no_error());

        require_rotation_matrix(extrin.rotation);     
        for(int i=0; i<3; ++i) REQUIRE(std::isfinite(extrin.translation[i]));
    }

    SECTION( "depth scale is 0.001 (by default)" )
    {
        float depth_scale = rs_get_device_depth_scale(dev, require_no_error());
        REQUIRE(depth_scale == 0.001f);
    }

    SECTION( "reasonable stream modes should be available for DEPTH, COLOR, INFRARED, and INFRARED2" )
    {
        for(auto stream : {RS_STREAM_DEPTH, RS_STREAM_COLOR, RS_STREAM_INFRARED})
        {
            // Require that there are modes for this stream
            int stream_mode_count = rs_get_stream_mode_count(dev, stream, require_no_error());
            REQUIRE(stream_mode_count > 0);

            // Require that INFRARED2 have the same number of modes as INFRARED
            if(stream == RS_STREAM_INFRARED)
            {
                int infrared2_stream_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_INFRARED2, require_no_error());
                REQUIRE(infrared2_stream_mode_count == stream_mode_count);
            }

            for(int i=0; i<stream_mode_count; ++i)
            {
                // Require that this mode has reasonable settings
                int width=0, height=0, framerate=0;
                rs_format format=RS_FORMAT_ANY;
                rs_get_stream_mode(dev, stream, i, &width, &height, &format, &framerate, require_no_error());
                REQUIRE(width >= 1);
                REQUIRE(width <= 1920);
                REQUIRE(height >= 1);
                REQUIRE(height <= 1080);
                REQUIRE(format > RS_FORMAT_ANY);
                REQUIRE(format < RS_FORMAT_COUNT);
                REQUIRE(framerate >= 15);
                REQUIRE(framerate <= 90);

                // Require that INFRARED2 have the exact same modes, in the same order, as INFRARED
                if(stream == RS_STREAM_INFRARED)
                {
                    int width2, height2, framerate2;
                    rs_format format2;
                    rs_get_stream_mode(dev, RS_STREAM_INFRARED2, i, &width2, &height2, &format2, &framerate2, require_no_error());
                    REQUIRE(width2 == width);
                    REQUIRE(height2 == height);
                    REQUIRE(format2 == format);
                    REQUIRE(framerate2 == framerate);
                }
            }

            // Require the mode requests with indices outside of [0,stream_mode_count) indicate an error
            int width=0, height=0, framerate=0;
            rs_format format=RS_FORMAT_ANY;
            rs_get_stream_mode(dev, stream, -1, &width, &height, &format, &framerate, require_error("out of range value for argument \"index\""));
            rs_get_stream_mode(dev, stream, stream_mode_count, &width, &height, &format, &framerate, require_error("out of range value for argument \"index\""));
        }
    }
}