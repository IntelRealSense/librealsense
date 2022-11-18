// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-defines.h>
#include <fastdds/rtps/common/Time_t.h>

#include <string>


namespace realdds {

    
inline dds_time now()
{
    dds_time t;
    dds_time::now( t );
    return t;
}


inline dds_time time_from( dds_nsec nanoseconds )
{
    dds_time t;
    t.from_ns( nanoseconds );
    return t;
}


// Easy way to format DDS time to a legible string, in milliseconds
//
class ms_s
{
    std::string _s;

public:
    enum abs_t { abs };
    enum rel_t { rel };

    enum no_suffix_t { no_suffix };

public:
    // absolute time
    ms_s( dds_nsec const t, abs_t, no_suffix_t )
        : _s( std::to_string( t / 1e6 ) )
    {
        if( _s.find( '.' ) != std::string::npos )
            while( _s.back() == '0' )
                _s.pop_back();
    }
    ms_s( dds_nsec const t, abs_t )  // with "ms" suffix by default
        : ms_s( t, abs, no_suffix )
    {
        _s += "ms";
    }

    // expressed as time-since-start (for readability)
    ms_s( dds_nsec const t, no_suffix_t )
        : ms_s( t - since_start(), abs, no_suffix ) {}
    ms_s( dds_nsec const t )
        : ms_s( t - since_start(), abs ) {}

    // relative time (so +/- followed by the number)
    ms_s( dds_nsec const delta, rel_t, no_suffix_t )
        : ms_s( delta, abs, no_suffix )
    {
        ensure_sign( delta );
    }
    ms_s( dds_nsec const delta, rel_t )  // with "ms" suffix by default
        : ms_s( delta, abs )
    {
        ensure_sign( delta );
    }

    // diff between two absolute times
    ms_s( dds_nsec const t, dds_nsec const start, no_suffix_t )
        : ms_s( t - start, rel, no_suffix ) {}
    ms_s( dds_nsec const t, dds_nsec const start )
        : ms_s( t - start, rel ) {}

    // Rest are variations of the above

    ms_s( realdds::dds_time const & t, realdds::dds_time const & start, no_suffix_t )
        : ms_s( t.to_ns(), start.to_ns(), no_suffix ) {}
    ms_s( realdds::dds_time const & t, realdds::dds_nsec const start, no_suffix_t )
        : ms_s( t.to_ns(), start, no_suffix ) {}
    ms_s( realdds::dds_nsec const t, realdds::dds_time const & start, no_suffix_t )
        : ms_s( t, start.to_ns(), no_suffix ) {}
    ms_s( realdds::dds_time const & t, no_suffix_t )
        : ms_s( t.to_ns(), no_suffix ) {}

    ms_s( realdds::dds_time const & t, realdds::dds_time const & start )
        : ms_s( t.to_ns(), start.to_ns() ) {}
    ms_s( realdds::dds_time const & t, realdds::dds_nsec const start )
        : ms_s( t.to_ns(), start ) {}
    ms_s( realdds::dds_nsec const t, realdds::dds_time const & start )
        : ms_s( t, start.to_ns() ) {}
    ms_s( realdds::dds_time const & t )
        : ms_s( t.to_ns() ) {}

    std::string const & to_string() const { return _s; }
    operator std::string const &() const { return to_string(); }

private:
    void ensure_sign( dds_nsec const delta )
    {
        if( delta > 0 )
            _s.insert( 0, 1, '+' );
    }
    static dds_nsec since_start()
    {
        static realdds::dds_nsec start = realdds::now().to_ns();
        return start;
    }
};


inline std::ostream & operator<<( std::ostream & os, ms_s const & ms )
    { return( os << (std::string) ms ); }
inline std::string operator+( std::string const & s, ms_s const & ms )
    { return s + (std::string) ms; }
inline std::string operator+( ms_s const & ms, std::string const & s )
    { return (std::string) ms + s; }


}  // namespace realdds
