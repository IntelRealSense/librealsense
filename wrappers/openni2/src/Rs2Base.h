#pragma once

#include <Driver/OniDriverAPI.h>

#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>

#include <vector>
#include <list>
#include <map>
#include <mutex>
#include <thread>

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4505) // unreferenced local function has been removed

#define ONI_MAX_DEPTH 10000
#define RS2_EMULATE_PRIMESENSE_HARDWARE // HACK: NiTE only runs on PrimeSense SoC

//#define RS2_TRACE_NOT_SUPPORTED_CMDS
//#define RS2_TRACE_NOT_SUPPORTED_PROPS

#if 0
	// DEV MODE
	#define PROF_ENABLED
	#define PROF_MUTEX_IMPL oni::driver::Rs2Mutex
	#define PROF_SCOPED_MUTEX_IMPL oni::driver::Rs2ScopedMutex
#else
	#define INIT_PROFILER
	#define SHUT_PROFILER
	#define NAMED_PROFILER(name)
	#define SCOPED_PROFILER
#endif

#if defined(NDEBUG)
	#define RS2_ASSERT(cond)
#else
	#define RS2_ASSERT assert
#endif

#define rsTraceError(format, ...) printf("[RS2] ERROR at FILE %s LINE %d FUNC %s\n\t" format "\n", __FILE__, __LINE__, __FUNCTION__, ##  __VA_ARGS__)
#define rsTraceFunc(format, ...) printf("[RS2] %s " format "\n", __FUNCTION__, ##  __VA_ARGS__)
#define rsLogDebug(format, ...) printf("[RS2] " format "\n", ## __VA_ARGS__)

namespace oni { namespace driver {

typedef std::mutex Rs2Mutex;
typedef std::lock_guard<std::mutex> Rs2ScopedMutex;

class Rs2Error
{
public:
	inline Rs2Error() : m_error(nullptr) {}
	inline ~Rs2Error() { release(); }
	inline void release() { if (m_error) { rs2_free_error(m_error); m_error = nullptr; } }
	inline rs2_error** operator&() { release(); return &m_error; }

	inline bool success() const { return m_error ? false : true; }
	inline const char* get_message() const { return rs2_get_error_message(m_error); }
	inline rs2_exception_type get_type() const { return rs2_get_librealsense_exception_type(m_error); }

private:
	rs2_error* m_error;
};

bool isSupportedStreamType(rs2_stream type);
OniSensorType convertStreamType(rs2_stream type);
rs2_stream convertStreamType(OniSensorType type);

bool isSupportedPixelFormat(rs2_format type);
int getPixelFormatBytes(rs2_format type);
OniPixelFormat convertPixelFormat(rs2_format type);
rs2_format convertPixelFormat(OniPixelFormat type);

}} // namespace

#if defined(PROF_ENABLED)
#include "Profiler.h"
#endif
