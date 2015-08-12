#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"

#include <stdexcept>

namespace rs
{
    // Transform 3D coordinates to pixel coordinate in an image
    inline void project_point_to_pixel(const float point[3], const rs_intrinsics & intrin, float pixel[2])
    {
        pixel[0] = intrin.principal_point[0] + intrin.focal_length[0] + point[0] / point[2];
        pixel[1] = intrin.principal_point[1] + intrin.focal_length[1] + point[1] / point[2];
    }

    // Transform pixel coordinates in an image to 3D coordinates
    inline void deproject_rectified_pixel_to_point(const float pixel[2], float depth, const rs_intrinsics & intrin, float point[3])
    {
        point[0] = depth * (pixel[0] - intrin.principal_point[0]) / intrin.focal_length[0];
        point[1] = depth * (pixel[1] - intrin.principal_point[1]) / intrin.focal_length[1];
        point[2] = depth;
    }

    // Transform 3D coordinates from one viewpoint to 3D coordinates in another viewpoint
    inline void transform_point_to_point(const float from_point[3], const rs_extrinsics & extrin, float to_point[3])
    {
        for(int i=0; i<3; ++i)
        {
            to_point[i] = extrin.translation_vec[i];
            for(int j=0; j<3; ++j) to_point[i] += extrin.rotation_matrix[i*3+j] * from_point[j];
        }
    }

    // Transform from pixel coordinates in one image to pixel coordinates in another image
    inline void transform_rectified_pixel_to_pixel(const float * from_pixel, float from_depth, const rs_intrinsics & from_intrin,
                                                   const rs_extrinsics & extrin, const rs_intrinsics & to_intrin, float * to_pixel)
    {
        float from_point[3], to_point[3];
        deproject_rectified_pixel_to_point(from_pixel, from_depth, from_intrin, from_point);
        transform_point_to_point(from_point, extrin, to_point);
        project_point_to_pixel(to_point, to_intrin, to_pixel);
    }

	// Modify this function if you wish to change error handling behavior from throwing exceptions to logging / callbacks / global status, etc.
	inline void handle_error(rs_error * error)
	{
		std::string message = std::string("error calling ") + rs_get_failed_function(error) + "(...):\n  " + rs_get_error_message(error);
		rs_free_error(error);
		throw std::runtime_error(message);
		// std::cerr << message << std::endl;
	}

	class auto_error
	{
		rs_error *			error;
	public:
							auto_error()															: error() {}
							auto_error(const auto_error &)											= delete;
							~auto_error()															{ if (error) handle_error(error); }
		auto_error &		operator = (const auto_error &)											= delete;
							operator rs_error ** ()													{ return &error; }
	};

    struct pixel { float x,y; };
    struct point { float x,y,z; };

    struct intrinsics : rs_intrinsics
    {

        point deproject(pixel p, float depth) const {}
        pixel project(point p) const {}
    };

    struct extrinsics : rs_extrinsics
    {
        point transform(point p) const {}
        point detransform(point p) const {}
    };



	class camera
	{
		rs_camera *			cam;
	public:
							camera()																: cam() {}
							camera(rs_camera * cam)													: cam(cam) {}
		explicit			operator bool() const													{ return !!cam; }
        rs_camera *         get_handle() const                                                      { return cam; }

		void				enable_stream(int stream)												{ return rs_enable_stream(cam, stream, auto_error()); }
		int 				configure_streams()														{ return rs_configure_streams(cam, auto_error()); }
		void				start_stream(int stream, int width, int height, int fps, int format)	{ return rs_start_stream(cam, stream, width, height, fps, format, auto_error()); }
		void				stop_stream(int stream)													{ return rs_stop_stream(cam, stream, auto_error()); }
		const uint16_t *	get_depth_image()														{ return rs_get_depth_image(cam, auto_error()); }
		const uint8_t *		get_color_image()														{ return rs_get_color_image(cam, auto_error()); }
		int 				is_streaming()															{ return rs_is_streaming(cam, auto_error()); }
		int					get_camera_index()														{ return rs_get_camera_index(cam, auto_error()); }
		uint64_t			get_frame_count()														{ return rs_get_frame_count(cam, auto_error()); }

		int					get_stream_property_i(int stream, int prop)								{ return rs_get_stream_property_i(cam, stream, prop, auto_error()); }
		float				get_stream_property_f(int stream, int prop)								{ return rs_get_stream_property_f(cam, stream, prop, auto_error()); }

        intrinsics          get_intrinsics(int stream);
        extrinsics          get_extrinsics(int from, int to);

	};

	class context
	{
		rs_context *		ctx;
	public:
							context()																: ctx(rs_create_context(RS_API_VERSION, auto_error())) {}
							context(const auto_error &)												= delete;
							~context()																{ rs_delete_context(ctx, nullptr); } // Deliberately ignore error on destruction
							context & operator = (const context &)									= delete;
        rs_context *        get_handle() const                                                      { return ctx; }

		int					get_camera_count()														{ return rs_get_camera_count(ctx, auto_error()); }
		camera				get_camera(int index)													{ return rs_get_camera(ctx, index, auto_error()); }
	};
}

#endif
