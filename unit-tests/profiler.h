#pragma once

#ifdef _WIN32
#include "windows.h"
#include "psapi.h"
#else
#endif


float mb_in_use()
{
    float mem = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof( pmc ) );

    mem = float( pmc.WorkingSetSize / (1024. * 1024.) );
#else
#endif
    return mem;
}
class memory_profiler
{
    float _baseline;
    std::atomic< float > _peak{ 0 };
    std::atomic_bool _alive{ true };

    std::thread _th;

    // section
    float _s_start = 0.f;
    std::atomic< float > _s_peak;

public:
    memory_profiler()
        : _baseline( mb_in_use() )
    {
        _th = std::thread( [&]() {
            while( _alive )
            {
                auto const mb = mb_in_use();
                _peak = std::max( mb, (float)_peak );
                _s_peak = std::max( mb, (float)_s_peak );
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            }
            } );
    }

    void section( char const * heading )
    {
        _s_start = mb_in_use();
        _s_peak = _s_start;
        TRACE( "\n\n-P------------ " << heading << ":" );
    }

    void section_end()
    {
        auto const mb = mb_in_use();
        _s_peak = std::max( mb, (float)_s_peak );
        TRACE( "-P------------ " << _s_start << " --> " << mb << "  /  " << _s_peak
            << " MB        delta  " << (mb - _s_start) << "  /  "
            << (_s_peak - _s_start) << "  peak\n" );
    }

    ~memory_profiler()
    {
        stop();

        auto const mb = mb_in_use();
        TRACE( "\n-P------------ FINAL: " << mb << "  /  " << _peak << " peak  (" << _baseline
            << " baseline)" );
    }

    void snapshot( char const * desc )
    {
        TRACE( "-P------------ " << desc << ": " << mb_in_use() << "  /  " << (float)_peak << " peak" );
    }

    void stop()
    {
        if( _alive )
        {
            _alive = false;
            _th.join();
        }
    }

    float get_peak() const { return _peak; }
    float get_baseline() const { return _baseline; }
};
