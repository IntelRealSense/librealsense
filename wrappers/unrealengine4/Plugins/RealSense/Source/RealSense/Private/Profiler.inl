#if defined(PROF_ENABLED)

static Profiler* _profiler = NULL;

void Profiler::InitInstance()
{
	PROF_SAFE_DELETE(_profiler);
	_profiler = new Profiler();
}

void Profiler::DestroyInstance()
{
	PROF_SAFE_DELETE(_profiler);
}

Profiler* Profiler::GetInstance()
{
	return _profiler;
}

ProfilerSection* Profiler::BeginSection(const PROF_CHAR* name, const PROF_CHAR* uniq)
{
	PROF_LOCK;

	ProfilerSection* section;
	SectionMap::iterator iter = m_sections.find(uniq);
	if (iter != m_sections.end())
	{
		section = iter->second;
	}
	else
	{
		section = new ProfilerSection(name, uniq);
		m_sections.insert(std::pair<const PROF_CHAR*, ProfilerSection*>(uniq, section));
	}

	return section;
}

void Profiler::EndSection(ProfilerSection* section, double delta)
{
	PROF_LOCK;

	section->Update(delta);
}

void Profiler::ResetCounters()
{
	PROF_LOCK;

	for (SectionMap::iterator iter = m_sections.begin(); iter != m_sections.end(); ++iter)
	{
		iter->second->ResetCounters();
	}
}

void Profiler::LogSummary(SortMode sortMode)
{
	std::vector<ProfilerSection*> sections;

	{
		PROF_LOCK;

		if (m_sections.empty())
		{
			return;
		}

		sections.reserve(m_sections.size());

		for (SectionMap::iterator iter = m_sections.begin(); iter != m_sections.end(); ++iter)
		{
			sections.push_back(iter->second);
		}
	}

	struct SectionComparatorSum
	{
		inline bool operator() (const ProfilerSection* a, const ProfilerSection* b) const { return a->GetSum() > b->GetSum(); }
	};

	struct SectionComparatorAvg
	{
		inline bool operator() (const ProfilerSection* a, const ProfilerSection* b) const { return a->GetAvg() > b->GetAvg(); }
	};

	if (sortMode == SORT_AVG)
		std::sort(sections.begin(), sections.end(), SectionComparatorAvg());
	else
		std::sort(sections.begin(), sections.end(), SectionComparatorSum());

	PROF_LOG(PROF_TEXT(""));
	PROF_LOG(PROF_TEXT("%10s %15s %10s %10s  %-20s"), PROF_TEXT("<sum s>"), PROF_TEXT("<sum ms>"), PROF_TEXT("<calls>"), PROF_TEXT("<avg ms>"), PROF_TEXT("<name>"));
	std::vector<ProfilerSection*>::iterator isec;
	for (isec = sections.begin(); isec != sections.end(); ++isec)
	{
		ProfilerSection* section = *isec;
		PROF_LOG(PROF_TEXT("%10.3f %15.3f %10u %10.3f  %-20s"), 
			(section->GetSum() * 0.001), section->GetSum(), section->GetCount(), section->GetAvg(), 
			section->GetName()); //, section->GetUniq());
	}
	PROF_LOG(PROF_TEXT(""));
}

void Profiler::Release()
{
	PROF_LOCK;

	for (SectionMap::iterator iter = m_sections.begin(); iter != m_sections.end(); ++iter)
	{
		delete iter->second;
	}

	m_sections.clear();
}

#if defined(PROF_PLATFORM_WINDOWS)

#if defined(PROF_OPTION_QPC)
	static double _perfFreqMsecInv = 0;
#else
	#pragma comment(lib, "Winmm.lib")
#endif

int HiresTimer::Init()
{
	#ifdef PROF_OPTION_QPC
		LARGE_INTEGER perfFreq;
		const BOOL rc = QueryPerformanceFrequency(&perfFreq);
		if (!rc || !perfFreq.QuadPart)
		{
			return -1;
		}

		_perfFreqMsecInv = 1.0 / ((double)perfFreq.QuadPart * 0.001);
	#endif

	return 0;
}

double HiresTimer::GetTicks()
{
	#ifdef PROF_OPTION_QPC
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return (counter.QuadPart * _perfFreqMsecInv);
	#else
		DWORD t = timeGetTime();
		return (double)t;
	#endif
}

#else // not PROF_PLATFORM_WINDOWS

int HiresTimer::Init()
{
	return 0;
}

double HiresTimer::GetTicks()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((ts.tv_sec * 1000.0) + (ts.tv_nsec * 0.000001));
}

#endif

#endif // PROF_ENABLED
