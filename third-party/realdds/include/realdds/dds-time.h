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
    // see eprosima::fastrtps::rtps::Time_t::from_ns() which includes unneeded fraction calculation
    auto res = std::lldiv( nanoseconds, 1000000000ull );
    if( res.rem < 0 )
    {
        --res.quot;
        res.rem += 1000000000ull;
    }
    return dds_time( static_cast< int32_t >( res.quot ),
                     static_cast< uint32_t >( res.rem ) );
}


// There's another version of Time_t that also calculates a fraction unnecessarily, so:
inline dds_time time_from( eprosima::fastrtps::rtps::Time_t const & rtps )
{
    return dds_time( rtps.seconds(), rtps.nanosec() );
}


inline long double time_to_double( dds_time const & t )
{
    long double sec = t.seconds;
    long double nsec = t.nanosec;
    nsec /= 1000000000ULL;
    return sec + nsec;
}


std::string time_to_string( dds_time const & t );


// Easy way to format DDS time to a legible string, in milliseconds
//
class timestr
{
    std::string _s;

public:
    enum abs_t { abs };
    enum rel_t { rel };

    enum no_suffix_t { no_suffix };

public:
    // absolute time
    timestr( dds_nsec const t, abs_t, no_suffix_t )
        : _s( std::to_string( t / 1e6 ) )
    {
        if( _s.find( '.' ) != std::string::npos )
            while( _s.back() == '0' )
                _s.pop_back();
    }
    timestr( dds_nsec const t, abs_t )  // with "ms" suffix by default
        : timestr( t, abs, no_suffix )
    {
        _s += "ms";
    }

    // expressed as time-since-start (for readability)
    timestr( dds_nsec const t, no_suffix_t )
        : timestr( t - since_start(), abs, no_suffix ) {}
    timestr( dds_nsec const t )
        : timestr( t - since_start(), abs ) {}

    // relative time (so +/- followed by the number)
    timestr( dds_nsec const delta, rel_t, no_suffix_t )
        : timestr( delta, abs, no_suffix )
    {
        ensure_sign( delta );
    }
    timestr( dds_nsec const delta, rel_t )  // with "ms" suffix by default
        : timestr( delta, abs )
    {
        ensure_sign( delta );
    }

    // diff between two absolute times
    timestr( dds_nsec const t, dds_nsec const start, no_suffix_t )
        : timestr( t - start, rel, no_suffix ) {}
    timestr( dds_nsec const t, dds_nsec const start )
        : timestr( t - start, rel ) {}

    // Rest are variations of the above

    timestr( realdds::dds_time const & t, realdds::dds_time const & start, no_suffix_t )
        : timestr( t.to_ns(), start.to_ns(), no_suffix ) {}
    timestr( realdds::dds_time const & t, realdds::dds_nsec const start, no_suffix_t )
        : timestr( t.to_ns(), start, no_suffix ) {}
    timestr( realdds::dds_nsec const t, realdds::dds_time const & start, no_suffix_t )
        : timestr( t, start.to_ns(), no_suffix ) {}
    timestr( realdds::dds_time const & t, no_suffix_t )
        : timestr( t.to_ns(), no_suffix ) {}

    timestr( realdds::dds_time const & t, realdds::dds_time const & start )
        : timestr( t.to_ns(), start.to_ns() ) {}
    timestr( realdds::dds_time const & t, realdds::dds_nsec const start )
        : timestr( t.to_ns(), start ) {}
    timestr( realdds::dds_nsec const t, realdds::dds_time const & start )
        : timestr( t, start.to_ns() ) {}
    timestr( realdds::dds_time const & t )
        : timestr( t.to_ns() ) {}

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


inline std::ostream & operator<<( std::ostream & os, timestr const & ms )
    { return( os << (std::string) ms ); }
inline std::string operator+( std::string const & s, timestr const & ms )
    { return s + (std::string) ms; }
inline std::string operator+( timestr const & ms, std::string const & s )
    { return (std::string) ms + s; }


}  // namespace realdds
