// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "ac-trigger.h"
#include "depth-to-rgb-calibration.h"
#include "l500-device.h"
#include "l500-color.h"
#include "l500-depth.h"
#include "algo/depth-to-rgb-calibration/debug.h"
#include "log.h"


namespace {
    template< class T >
    class env_var
    {
        bool _is_set;
        T _value;

    public:
        template< class X >
        struct string_to
        {
        };

        template<>
        struct string_to< std::string >
        {
            static std::string convert( std::string const & s )
            {
                return s;
            }
        };

        template<>
        struct string_to< int >
        {
            static int convert( std::string const & s )
            {
                char * p_end;
                auto v = std::strtol( s.data(), &p_end, 10 );
                if( errno == ERANGE )
                    throw std::out_of_range( "out of range" );
                if( p_end != s.data() + s.length() )
                    throw std::invalid_argument( "extra characters" );
                return v;
            }
        };

        env_var( char const * name, T default_value, std::function< bool( T ) > checker = nullptr )
        {
            auto lpsz = getenv( name );
            _is_set = (lpsz != nullptr);
            if( _is_set )
            {
                try
                {
                    _value = string_to< T >::convert( lpsz );
                    if( checker && ! checker( _value ) )
                        throw std::invalid_argument( "does not check" );
                }
                catch( std::exception const & e )
                {
                    AC_LOG( ERROR,
                            "Environment variable \"" << name << "\" is set, but its value (\""
                                                      << lpsz << "\") is invalid (" << e.what()
                                                      << "); using default of \"" << default_value
                                                      << "\"" );
                    _is_set = false;
                }
            }
            if( !_is_set )
                _value = default_value;
        }

        bool is_set() const { return _is_set; }
        T value() const { return _value; }

        operator T() const { return _value; }
    };
}


namespace librealsense {
namespace ivcam2 {


    ac_trigger::enabler_option::enabler_option( std::shared_ptr< ac_trigger > const & autocal )
        : bool_option( false )
        , _autocal( autocal )
    {
    }

    void ac_trigger::enabler_option::set( float value )
    {
        bool_option::set( value );
        if( is_true() )
        {
            // We've turned it on -- try to immediately get a special frame
            _autocal->trigger_special_frame();
        }
        else if( _autocal->_to_profile )
        {
            // TODO remove before release
            // Reset the calibration so we can do it all over again
            if( auto color_sensor = _autocal->_dev.get_color_sensor() )
                color_sensor->reset_calibration();
            _autocal->_dev.get_depth_sensor().reset_calibration();
#if 1
            _autocal->_dev.notify_of_calibration_change( RS2_CALIBRATION_SUCCESSFUL );
#else
            // Make sure we have the new setting before calling the callback
            //auto&& active_streams = get_active_streams();
            //if( !active_streams.empty() )
            {
                //std::shared_ptr< stream_profile_interface > spi = active_streams.front();
                auto vspi
                    = dynamic_cast<video_stream_profile_interface *>(_autocal->_to_profile);
                rs2_intrinsics const _intr = vspi->get_intrinsics();
                AC_LOG( DEBUG, "reset intrinsics to: [ " << AC_F_PREC
                    << _intr.width << "x" << _intr.height
                    << "  ppx: " << _intr.ppx << ", ppy: " << _intr.ppy << ", fx: " << _intr.fx
                    << ", fy: " << _intr.fy << ", model: " << int( _intr.model ) << " coeffs["
                    << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2]
                    << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4] << "] ]" );
                environment::get_instance().get_extrinsics_graph().
                    try_fetch_extrinsics( *_autocal->_from_profile, *_autocal->_to_profile, &_autocal->_extr );
                AC_LOG( DEBUG, "reset extrinsics to: " << _autocal->_extr );
                _autocal->_dsm_params = _autocal->_dev.get_depth_sensor().get_dsm_params();
                _autocal->_dev.notify_of_calibration_change( RS2_CALIBRATION_SUCCESSFUL );
            }
#endif
        }
        _record_action( *this );
    }


    // Implementation class: starts another thread responsible for sending a retry
    // NOTE that it does this as long as our shared_ptr keeps us alive: once it's gone, if the
    // retry period elapses then nothing will happen!
    class ac_trigger::retrier
    {
        ac_trigger & _ac;

        retrier( ac_trigger & ac )
            : _ac( ac )
        {
        }

        void retry()
        {
            AC_LOG( DEBUG, "retrying " << std::hex << this << std::dec );
            _ac.trigger_special_frame( true );
        }

    public:
        ~retrier()
        {
            AC_LOG( DEBUG, "~retrier " << std::hex << this << std::dec );
        }

        static std::shared_ptr< retrier > start( ac_trigger & autocal, std::chrono::seconds n_seconds )
        {
            auto r = new retrier( autocal );
            AC_LOG( DEBUG, "retrier " << std::hex << r << std::dec << " " << n_seconds.count() << " seconds starting" );
            auto pr = std::shared_ptr< retrier >( r );
            std::weak_ptr< retrier > weak{ pr };
            std::thread( [r, weak, n_seconds]()
                {
                    std::this_thread::sleep_for( n_seconds );
                    if( auto pr = weak.lock() )
                        pr->retry();
                    else
                        AC_LOG( DEBUG, "retrier " << std::hex << r << std::dec
                            << " " << n_seconds.count() << " seconds are up; no retry needed" );
                } ).detach();
                return pr;
        };
    };


    /*
        Temporary (?) class used to direct AC logs to either console or a special log

        If RS2_DEBUG_DIR is defined in the environment, we try to create a log file in there
        that has the name "<pid>.ac_log".
    */
    class ac_logger : public rs2_log_callback
    {
        std::ofstream _f;
        bool _to_stdout;

    public:
        explicit ac_logger( bool to_stdout = false )
            : _to_stdout( to_stdout )
        {
            using namespace std::chrono;
            auto dir = getenv( "RS2_DEBUG_DIR" );
            if( dir )
            {
                std::string filename = to_string()
                    << dir
                    << system_clock::now().time_since_epoch().count() * system_clock::period::num / system_clock::period::den
                    << ".ac_log";

                _f.open( filename );
                if( _f )
                    std::cout << "-D- AC log is being written to: " << filename << std::endl;
            }

            librealsense::log_to_callback( RS2_LOG_SEVERITY_ALL,
                { this, []( rs2_log_callback * p ) {} } );
        }

        void on_log( rs2_log_severity severity, rs2_log_message const & msg ) noexcept override
        {
            log_message const & wrapper = (log_message const &)(msg);
            char const * raw = wrapper.el_msg.message().c_str();
            if( strncmp( "AC1: ", raw, 5 ) )
                return;
            std::ostringstream ss;
            ss << "-" << "DIWE"[severity] << "- ";
            ss << (raw + 5);
            std::string text = ss.str();
            std::cout << text << std::endl;
            if( _f )
                _f << text << std::endl;
        }

        void release() override { delete this; }
    };


    ac_trigger::ac_trigger( l500_device & dev, hw_monitor & hwm )
        : _is_processing{ false }
        , _hwm( hwm )
        , _dev( dev )
        , _from_profile( nullptr )
        , _to_profile( nullptr )
    {
        bool to_stdout = true;
        static ac_logger one_logger(
#if 1  // TODO remove this -- but for now, it's an easy way of seeing the AC stuff
            true  // log to stdout
#endif
            );
    }


    ac_trigger::~ac_trigger()
    {
        if( _worker.joinable() )
        {
            _is_processing = false;  // Signal the thread that we want to stop!
            _worker.join();
        }
    }


    void ac_trigger::trigger_special_frame( bool is_retry )
    {
        _retrier.reset();
        if( _is_processing )
        {
            AC_LOG( ERROR, "Failed to trigger_special_frame: auto-calibration is already in-process" );
            return;
        }
        if( is_retry )
        {
            if( _recycler )
            {
                // This is another cycle of AC, after we've woken up from some time after
                // the previous invalid-scene or bad-result...
                _n_retries = 0;
                _recycler.reset();
            }
            else if( ++_n_retries > 4 )
            {
                AC_LOG( ERROR, "too many retries; aborting" );
                call_back( RS2_CALIBRATION_FAILED );
                return;
            }
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle " << (_n_cycles + 1) << " retry " << _n_retries << ")" );
            call_back( RS2_CALIBRATION_RETRY );
        }
        else
        {
            _n_retries = 0;
            _n_cycles = 0;
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle 1)" );
        }
        command cmd{ GET_SPECIAL_FRAME, 0x5F, 1 };  // 5F = SF = Special Frame, for easy recognition
        auto res = _hwm.send( cmd );
        // Start a timer: enable retries if something's wrong with the special frame
        int n_seconds
            = env_var< int >( "RS2_AC_SF_RETRY_SECONDS", 2, []( int n ) { return n > 0; } );
        _retrier = retrier::start( *this, std::chrono::seconds( n_seconds ) );
    }


    void ac_trigger::set_special_frame( rs2::frameset const& fs )
    {
        AC_LOG( DEBUG, "special frame received :)" );
        // Notify of the special frame -- mostly for validation team so they know to expect a frame drop...
        call_back( RS2_CALIBRATION_SPECIAL_FRAME );

        if( _is_processing )
        {
            AC_LOG( ERROR, "already processing; ignoring special frame!" );
            return;
        }
        auto irf = fs.get_infrared_frame();
        if( !irf )
        {
            AC_LOG( ERROR, "no IR frame found; ignoring special frame!" );
            //call_back( RS2_CALIBRATION_FAILED );
            return;
        }
        auto df = fs.get_depth_frame();
        if( !df )
        {
            AC_LOG( ERROR, "no depth frame found; ignoring special frame!" );
            //call_back( RS2_CALIBRATION_FAILED );
            return;
        }

        _retrier.reset();  // No need to activate a retry if the following takes a bit of time!

        // We have to read the FW registers at the time of the special frame.
        // NOTE: the following is I/O to FW, meaning it takes time! In this time, another
        // thread can receive a set_color_frame() and, since we've already received the SF,
        // start working even before we finish! NOT GOOD!
        ivcam2::read_fw_register( _hwm, &_dsm_x_scale, 0xfffe3844 );
        ivcam2::read_fw_register( _hwm, &_dsm_y_scale, 0xfffe3830 );
        ivcam2::read_fw_register( _hwm, &_dsm_x_offset, 0xfffe3840 );
        ivcam2::read_fw_register( _hwm, &_dsm_y_offset, 0xfffe382c );
        AC_LOG( DEBUG, "dsm registers=  x[" << AC_F_PREC << _dsm_x_scale << ' ' << _dsm_y_scale
            << "]  +[" << _dsm_x_offset << ' ' << _dsm_y_offset
            << "]" );

        _sf = fs;  // Assign right before the sync otherwise we may start() prematurely
        std::lock_guard< std::mutex > lock( _mutex );
        if( check_color_depth_sync() )
            start();
    }


    void ac_trigger::set_color_frame( rs2::frame const& f )
    {
        if( _is_processing )
            // No error message -- we expect to get new color frames while processing...
            return;

        _pcf = _cf;
        _cf = f;
        std::lock_guard< std::mutex > lock( _mutex );
        if( check_color_depth_sync() )
            start();
    }


    bool ac_trigger::check_color_depth_sync()
    {
        // Only one thread is allowed to start(), and _is_processing is set within
        // the mutex!
        if( _is_processing )
            return false;

        if( !_sf )
        {
            //std::cout << "-D- no special frame yet" << std::endl;
            return false;
        }
        if( !_cf )
        {
            AC_LOG( DEBUG, "no color frame received yet" );
            return false;
        }
        return true;
    }


    void ac_trigger::start()
    {
        AC_LOG( DEBUG, "starting processing ..." );
        _is_processing = true;
        if( _worker.joinable() )
        {
            AC_LOG( DEBUG, "waiting for worker to join ..." );
            _worker.join();
        }
        _worker = std::thread( [&]()
            {
                try
                {
                    AC_LOG( DEBUG, "auto calibration has started ..." );
                    call_back( RS2_CALIBRATION_STARTED );

                    static algo::depth_to_rgb_calibration::algo_calibration_info cal_info;
                    static bool cal_info_initialized = false;
                    if( !cal_info_initialized )
                    {
                        cal_info_initialized = true;
                        ivcam2::read_fw_table( _hwm, cal_info.table_id, &cal_info );  // throws!
                    }
                    algo::depth_to_rgb_calibration::algo_calibration_registers cal_regs;
                    cal_regs.EXTLdsmXscale = _dsm_x_scale;
                    cal_regs.EXTLdsmYscale = _dsm_y_scale;
                    cal_regs.EXTLdsmXoffset = _dsm_x_offset;
                    cal_regs.EXTLdsmYoffset = _dsm_y_offset;

                    auto df = _sf.get_depth_frame();
                    auto irf = _sf.get_infrared_frame();
                    depth_to_rgb_calibration algo( df, irf, _cf, _pcf, cal_info, cal_regs );
                    _from_profile = algo.get_from_profile();
                    _to_profile = algo.get_to_profile();

                    auto status =
                        algo.optimize( [this]( rs2_calibration_status status ) { call_back( status ); } );
                    switch( status )
                    {
                    case RS2_CALIBRATION_SUCCESSFUL:
                        _extr = algo.get_extrinsics();
                        _intr = algo.get_intrinsics();
                        _dsm_params = algo.get_dsm_params();
                        // Fall-thru!
                    case RS2_CALIBRATION_NOT_NEEDED:
                        // This is the same as SUCCESSFUL, except there was no change because the
                        // existing calibration is good enough. We notify and exit.
                        call_back( status );
                        break;
                    case RS2_CALIBRATION_RETRY:
                        // We want to trigger a special frame after a certain longer delay
                        if( ++_n_cycles > 4 )
                        {
                            // ... but we've tried too many times
                            AC_LOG( ERROR, "Too many cycles of calibration; quitting" );
                            call_back( RS2_CALIBRATION_FAILED );
                        }
                        else
                        {
                            AC_LOG( DEBUG, "Triggering another cycle for calibration..." );
                            int n_seconds = env_var< int >( "RS2_AC_INVALID_RETRY_SECONDS",
                                                            60,
                                                            []( int n ) { return n > 0; } );
                            _recycler = retrier::start( *this, std::chrono::seconds( n_seconds ) );
                        }
                        break;
                    case RS2_CALIBRATION_FAILED:
                        // Report it and quit: this is a final status
                        call_back( status );
                        break;
                    default:
                        // All the rest of the codes are not end-states of the algo, so we don't expect
                        // to get here!
                        AC_LOG( ERROR,
                            "Unexpected status '" << status << "' received from AC algo; stopping!" );
                        call_back( RS2_CALIBRATION_FAILED );
                        break;
                    }
                }
                catch( std::exception& ex )
                {
                    AC_LOG( ERROR, "caught exception: " << ex.what() );
                    call_back( RS2_CALIBRATION_FAILED );
                }
                catch( ... )
                {
                    AC_LOG( ERROR, "unknown exception in AC!!!" );
                    call_back( RS2_CALIBRATION_FAILED );
                }
                reset();  // Must happen before any retries
            } );
    }

    void ac_trigger::reset()
    {
        _sf = rs2::frame{};
        _cf = rs2::frame{};;
        _pcf = rs2::frame{};

        _is_processing = false;
        AC_LOG( DEBUG, "reset()" );
    }


    ac_trigger::depth_processing_block::depth_processing_block(
        std::shared_ptr< ac_trigger > autocal
    )
        : generic_processing_block( "Auto Calibration (depth)" )
        , _autocal{ autocal }
    {
    }


    static bool is_special_frame( rs2::depth_frame const& f )
    {
        if( !f )
            return false;

        if( f.supports_frame_metadata( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE ) )
        {
            // We specified 0x5F (SF = Special Frame) when we asked for the SPECIAL_FRAME
            auto mode = f.get_frame_metadata( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE );
            if( 0x5F == mode )
            {
                AC_LOG( DEBUG, "frame " << f.get_frame_number() << " is our special frame" );
                return true;
            }
            return false;
        }

        // When the frame doesn't support metadata, we have to look at its
        // contents: we take the first frame that has 100% fill rate, i.e.
        // has no pixels with 0 in them...
        uint16_t const * pd = reinterpret_cast<const uint16_t *>(f.get_data());
        for( size_t x = f.get_data_size() / sizeof( *pd ); x--; ++pd )
        {
            if( !*pd )
                return false;
        }
        AC_LOG( DEBUG, "frame " << f.get_frame_number() << " has no metadata but 100% fill rate -> assuming special frame" );
        return true;  // None zero, assume special frame...
    }

    rs2::frame ac_trigger::depth_processing_block::process_frame( const rs2::frame_source & source,
                                                                  const rs2::frame & f )
    {
        // AC can be triggered manually, too, so we do NOT check whether the option is on!

        auto fs = f.as< rs2::frameset >();
        if( fs )
        {
            auto df = fs.get_depth_frame();
            if( _autocal->is_expecting_special_frame() && is_special_frame( df ) )
                _autocal->set_special_frame( f );
            // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
            return rs2::frame{};
        }

        if( _autocal->is_expecting_special_frame() && is_special_frame( f.as< rs2::depth_frame >() ) )
            // We don't want the user getting this frame!
            return rs2::frame{};

        return f;
    }


    bool ac_trigger::depth_processing_block::should_process( const rs2::frame & frame )
    {
        return true;
    }


    rs2::frame ac_trigger::depth_processing_block::prepare_output(
        const rs2::frame_source & source, rs2::frame input, std::vector< rs2::frame > results )
    {
        // The default prepare_output() will send the input back as the frame if the results are empty
        if( results.empty() )
            return rs2::frame{};

        return source.allocate_composite_frame( results );
    }


    ac_trigger::color_processing_block::color_processing_block(
        std::shared_ptr< ac_trigger > autocal
    )
        : generic_processing_block( "Auto Calibration (color)" )
        , _autocal{ autocal }
    {
    }


    rs2::frame ac_trigger::color_processing_block::process_frame( const rs2::frame_source& source, const rs2::frame& f )
    {
        // AC can be triggered manually, too, so we do NOT check whether the option is on!

        // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
        if( f.is< rs2::frameset >() )
            return rs2::frame{};

        // We record each and every color frame
        _autocal->set_color_frame( f );

        // Return the frame as is!
        return f;
    }


    bool ac_trigger::color_processing_block::should_process( const rs2::frame& frame )
    {
        return true;
    }


}  // namespace ivcam2
}  // namespace librealsense
