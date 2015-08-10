#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"

#include <stdexcept>

namespace rs
{
	// Modify this function if you wish to change error handling behavior from throwing exceptions to logging / callbacks / global status, etc.
	inline void handle_error(rs_error * error)
	{
		std::string message = std::string("error calling ") + rs_get_failed_function(error) + "(...) - " + rs_get_error_message(error);
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

	class camera
	{
		rs_camera *			cam;
	public:
							camera()																: cam() {}
							camera(rs_camera * cam)													: cam(cam) {}
		explicit			operator bool() const													{ return !!cam; }

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
	};

	class context
	{
		rs_context *		ctx;
	public:
							context()																: ctx(rs_create_context(auto_error())) {}
							context(const auto_error &)												= delete;
							~context()																{ rs_delete_context(ctx, nullptr); } // Deliberately ignore error on destruction
							context & operator = (const context &)									= delete;

		int					get_camera_count()														{ return rs_get_camera_count(ctx, auto_error()); }
		camera				get_camera(int index)													{ return rs_get_camera(ctx, index, auto_error()); }
	};
}

#endif