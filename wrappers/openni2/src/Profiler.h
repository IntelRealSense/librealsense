#pragma once

#if defined(PROF_ENABLED)

#define PROF_OPTION_QPC
#define PROF_OPTION_THREADSAFE

#define INIT_PROFILER for(;;) { HiresTimer::Init(); Profiler::InitInstance(); break; }
#define SHUT_PROFILER for(;;) { Profiler::GetInstance()->LogSummary(); Profiler::DestroyInstance(); break; }
#define NAMED_PROFILER(name) ScopedProfiler named_profiler(PROF_TEXT(name), PROF_TEXT(PROF_UNIQ_NAME))
#define SCOPED_PROFILER ScopedProfiler scoped_profiler(PROF_TEXT(__FUNCTION__), PROF_TEXT(PROF_UNIQ_NAME))

#if defined(_WIN32) || defined(_WIN64)
	#define PROF_PLATFORM_WINDOWS
#endif

#include <vector>
#include <algorithm>

#if !defined(PROF_PLATFORM_WINDOWS)
	#include <sys/time.h>
	#include <time.h>
#endif

#if !defined(PROF_CHAR)
	#define PROF_CHAR char
#endif

#if !defined(PROF_TEXT)
	#define PROF_TEXT(str) str
#endif

#if !defined(PROF_LOG)
	#define PROF_LOG printf
#endif

#if !defined(PROF_MAP_IMPL)
	#define PROF_MAP_IMPL std::unordered_map
	#include <unordered_map>
#endif

#if defined(PROF_OPTION_THREADSAFE)
	#if !defined(PROF_MUTEX_IMPL)
		#include <mutex>
		#define PROF_MUTEX_IMPL std::mutex
		#define PROF_SCOPED_MUTEX_IMPL std::lock_guard<std::mutex>
	#endif
	#define PROF_MUTEX_DECL PROF_MUTEX_IMPL m_mutex
	#define PROF_LOCK PROF_SCOPED_MUTEX_IMPL _scoped(m_mutex)
#else
	#define PROF_MUTEX_DECL
	#define PROF_LOCK
#endif

#define PROF_STRINGIZE1(x) PROF_STRINGIZE2(x)
#define PROF_STRINGIZE2(x) #x
#define PROF_UNIQ_NAME __FILE__ "_" PROF_STRINGIZE1(__LINE__)

#define PROF_SAFE_DELETE(ptr) { if (ptr) { delete ptr; ptr = nullptr; } }

#define PROF_NO_COPY(type_)\
	private:\
	type_(const type_&); \
	void operator=(const type_&)

// HiresTimer

class HiresTimer
{
public:
	static int Init();
	static double GetTicks();
};

// ProfilerSection

class ProfilerSection
{
	PROF_NO_COPY(ProfilerSection);

	friend class Profiler;
	friend class ScopedProfiler;

private:

	inline ProfilerSection(const PROF_CHAR* name, const PROF_CHAR* uniq) :
		m_name(name), m_uniq(uniq), m_sum(0), m_count(0) {}

	inline void Update(double delta) { m_sum += delta, m_count += 1; }
	inline void ResetCounters() { m_sum = 0, m_count = 0; }

	inline const PROF_CHAR* GetName() const { return m_name; }
	inline const PROF_CHAR* GetUniq() const { return m_uniq; }
	inline double GetSum() const { return m_sum; }
	inline unsigned int GetCount() const { return m_count; }
	inline double GetAvg() const { return (m_count > 0 ? (m_sum / (double)m_count) : 0); }

private:

	const PROF_CHAR* m_name;
	const PROF_CHAR* m_uniq;
	double m_sum;
	unsigned int m_count;
};

// Profiler

class Profiler
{
	PROF_NO_COPY(Profiler);

public:

	enum SortMode
	{
		SORT_SUM = 0,
		SORT_AVG
	};

	static void InitInstance();
	static void DestroyInstance();
	static Profiler* GetInstance();

	Profiler() {}
	~Profiler() { Release(); }

	ProfilerSection* BeginSection(const PROF_CHAR* name, const PROF_CHAR* uniq);
	void EndSection(ProfilerSection* section, double delta);
	void ResetCounters();

	void LogSummary(SortMode sortMode = SORT_SUM);
	void Release();

private:

	PROF_MUTEX_DECL;

	typedef PROF_MAP_IMPL<const PROF_CHAR*, ProfilerSection*> SectionMap;
	SectionMap m_sections;
};

// ScopedProfiler

class ScopedProfiler
{
	PROF_NO_COPY(ScopedProfiler);

public:

	inline ScopedProfiler(const PROF_CHAR* name, const PROF_CHAR* uniq)
	{
		m_section = Profiler::GetInstance()->BeginSection(name, uniq);
		m_ticks = HiresTimer::GetTicks();
	}

	inline ~ScopedProfiler()
	{
		const double ticks = HiresTimer::GetTicks();
		const double delta = (ticks - m_ticks);
		Profiler::GetInstance()->EndSection(m_section, delta);
	}

private:

	ProfilerSection* m_section;
	double m_ticks;
};

#endif // PROF_ENABLED
