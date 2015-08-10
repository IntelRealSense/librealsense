#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"

#include <stdexcept>

namespace rs
{
	// Modify this function if you wish to change error handling behavior from throwing exceptions to logging / callbacks / global status, etc.
	inline void handle_error(RSerror error)
	{
		std::string message = std::string("error calling ") + rsGetFailedFunction(error) + "(...) - " + rsGetErrorMessage(error);
		rsFreeError(error);
		throw std::runtime_error(message);
		// std::cerr << message << std::endl;
	}

	class auto_error
	{
		RSerror				error;
	public:
							auto_error() : error() {}
							auto_error(const auto_error &) = delete;
							~auto_error() { if (error) handle_error(error); }
		auto_error &		operator = (const auto_error &) = delete;
							operator RSerror * () { return &error; }
	};

	class camera
	{
		RScamera			cam;
	public:
							camera() : cam() {}
							camera(RScamera cam) : cam(cam) {}
		explicit			operator bool() const { return cam; }

		void				enable_stream(RSenum stream) { return rsEnableStream(cam, stream, auto_error()); }
		int 				configure_streams() { return rsConfigureStreams(cam, auto_error()); }
		void				start_stream(RSenum stream, int width, int height, int fps, RSenum format) { return rsStartStream(cam, stream, width, height, fps, format, auto_error()); }
		void				stop_stream(RSenum stream) { return rsStopStream(cam, stream, auto_error()); }
		const uint16_t *	get_depth_image() { return rsGetDepthImage(cam, auto_error()); }
		const uint8_t *		get_color_image() { return rsGetColorImage(cam, auto_error()); }
		int 				is_streaming() { return rsIsStreaming(cam, auto_error()); }
		int					get_camera_index() { return rsGetCameraIndex(cam, auto_error()); }
		uint64_t			get_frame_count() { return rsGetFrameCount(cam, auto_error()); }

		int					get_stream_propertyi(RSenum stream, RSenum prop) { return rsGetStreamPropertyi(cam, stream, prop, auto_error()); }
		float				get_stream_propertyf(RSenum stream, RSenum prop) { return rsGetStreamPropertyf(cam, stream, prop, auto_error()); }
	};

	class context
	{
		RScontext			ctx;
	public:
							context() : ctx(rsCreateContext(auto_error())) {}
							context(const auto_error &) = delete;
							~context() { rsDeleteContext(ctx, nullptr); } // Deliberately ignore error on destruction
							context & operator = (const context &) = delete;

		int					get_camera_count() { return rsGetCameraCount(ctx, auto_error()); }
		camera				get_camera(int index) { return rsGetCamera(ctx, index, auto_error()); }
	};
}

#endif