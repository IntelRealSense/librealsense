// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "ac-trigger.h"
#include "depth-to-rgb-calibration.h"
#include "l500-device.h"
#include "l500-color.h"
#include "l500-depth.h"
#include "algo/depth-to-rgb-calibration/debug.h"
#include "log.h"



template < class X > struct string_to {};

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


template< class T >
class env_var
{
    bool _is_set;
    T _value;

public:
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


static int get_retry_sf_seconds()
{
    static int n_seconds
        = env_var< int >( "RS2_AC_SF_RETRY_SECONDS", 2, []( int n ) { return n > 0; } );
    return n_seconds;
}
static double get_temp_diff_trigger()
{
    static double d_temp
        = env_var< int >( "RS2_AC_TEMP_DIFF", 5, []( int n ) { return n >= 0; } ).value();
    return d_temp;
}
static std::chrono::seconds get_trigger_seconds()
{
    auto n_seconds = env_var< int >( "RS2_AC_TRIGGER_SECONDS",
        600,  // 10 minutes since last
        []( int n ) { return n >= 0; } );
    // 0 means turn off auto-trigger
    return std::chrono::seconds( n_seconds );
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
            _autocal->trigger_calibration();
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
        unsigned _id;
        char const * const _name;

    protected:
        retrier( ac_trigger & ac, char const * name )
            : _ac( ac )
            , _name( name ? name : "retrier" )
        {
            static unsigned id = 0;
            _id = ++id;
        }

        unsigned get_id() const { return _id; }
        ac_trigger & get_ac() const { return _ac; }
        char const * get_name() const { return _name; }

        virtual void retry()
        {
            AC_LOG( DEBUG, "retrying " << get_id() );
            _ac.trigger_calibration( true );
        }

    public:
        virtual ~retrier()
        {
            AC_LOG( DEBUG, "~" << get_name() << " " << get_id() );
        }

        template < class T = retrier >
        static std::shared_ptr< T > start( ac_trigger & trigger,
                                           std::chrono::seconds n_seconds,
                                           const char * name = nullptr )
        {
            T * r = new T( trigger, name );
            auto id = r->get_id();
            name = r->get_name();
            AC_LOG( DEBUG, name << ' ' << id << ": " << n_seconds.count() << " seconds starting" );
            auto pr = std::shared_ptr< T >( r );
            std::weak_ptr< T > weak{ pr };
            std::thread( [=]() {
                std::this_thread::sleep_for( n_seconds );
                auto pr = weak.lock();
                if( pr && pr->get_id() == id )
                    ( (retrier *)pr.get() )->retry();
                else
                    AC_LOG( DEBUG,
                            name << ' ' << id << ": " << n_seconds.count()
                                 << " seconds are up; nothing needed" );
            } ).detach();
            return pr;
        };
    };
    class ac_trigger::temp_check : public ac_trigger::retrier
    {
    public:
        temp_check( ac_trigger & ac, const char * name )
            : retrier( ac, name ? name : "temp check" )
        {
        }

    private:
        void retry() override
        {
            auto & trigger = get_ac();
            if( trigger.is_active() )
            {
                AC_LOG( DEBUG, "temp check " << get_id() << ": AC already active" );
                return;
            }
            auto current_temp = trigger.read_temperature();
            auto d_temp = current_temp - trigger._last_temp;
            if( d_temp >= get_temp_diff_trigger() )
            {
                AC_LOG( DEBUG, "Delta since last calibration is " << d_temp << " degrees Celsius; triggering..." );
                trigger.trigger_calibration();
            }
            else
            {
                // We do not update the trigger temperature: that is only updated after calibration
                AC_LOG( DEBUG, "Delta since last calibration is " << d_temp << " degrees Celsius" );
                trigger._temp_check = retrier::start< temp_check >( trigger, std::chrono::seconds( 60 ) );
            }
        }
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
        : _hwm( hwm )
        , _dev( dev )
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


    void ac_trigger::trigger_calibration( bool is_retry )
    {
        _retrier.reset();
        if( is_retry  &&  is_active() )
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
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle " << _n_cycles << " retry " << _n_retries << ")" );
            call_back( RS2_CALIBRATION_RETRY );
        }
        else
        {
            if( is_active() )
            {
                AC_LOG( ERROR, "Failed to trigger calibration: AC is already active" );
                return;
            }
            _n_retries = 0;
            _n_cycles = 1;          // now active
            _next_trigger.reset();  // don't need a trigger any more
            _temp_check.reset();    // nor a temperature check
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle 1); now active..." );
        }
        command cmd{ GET_SPECIAL_FRAME, 0x5F, 1 };  // 5F = SF = Special Frame, for easy recognition
        auto res = _hwm.send( cmd );
        // Start a timer: enable retries if something's wrong with the special frame
        _retrier = retrier::start( *this, std::chrono::seconds( get_retry_sf_seconds() ) );
    }


    void ac_trigger::set_special_frame( rs2::frameset const & fs )
    {
        if( !is_active() )
        {
            AC_LOG( ERROR, "Special frame received while is_active() is false" );
            return;
        }

        AC_LOG( DEBUG, "special frame received :)" );
        // Notify of the special frame -- mostly for validation team so they know to expect a frame
        // drop...
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
        _sf.keep();
        std::lock_guard< std::mutex > lock( _mutex );
        if( check_color_depth_sync() )
            run_algo();
        else
            _retrier = retrier::start( *this, std::chrono::seconds( get_retry_sf_seconds() ) );
    }


    void ac_trigger::set_color_frame( rs2::frame const& f )
    {
        if( ! is_active() )
            // No error message -- we expect to get new color frames while processing...
            return;

        _pcf = _cf;
        _cf = f;
        _cf.keep();
        std::lock_guard< std::mutex > lock( _mutex );
        if( check_color_depth_sync() )
            run_algo();
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
            AC_LOG( DEBUG, "no color frame received; maybe color stream isn't on?" );
            return false;
        }
        return true;
    }


    void ac_trigger::run_algo()
    {
        AC_LOG( DEBUG, "Starting processing ..." );
        _is_processing = true;
        _retrier.reset();
        if( _worker.joinable() )
        {
            AC_LOG( DEBUG, "Waiting for worker to join ..." );
            _worker.join();
        }
        _worker = std::thread(
            [&]() {
                try
                {
                    AC_LOG( DEBUG, "Calibration algo has started ..." );
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

                    auto status = algo.optimize(
                        [this]( rs2_calibration_status status ) { call_back( status ); } );

                    // It's possible that, while algo was working, stop() was called. In this case,
                    // we have to make sure that we notify of failure:
                    if( !is_active() )
                    {
                        AC_LOG( DEBUG, "Algo finished (with " << status << "), but stop() was detected; notifying of failure..." );
                        status = RS2_CALIBRATION_FAILED;
                    }

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
                        if( ++_n_cycles > 5 )
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
                    AC_LOG( ERROR, "caught exception in calibration algo: " << ex.what() );
                    call_back( RS2_CALIBRATION_FAILED );
                }
                catch( ... )
                {
                    AC_LOG( ERROR, "unknown exception in calibration algo!!!" );
                    call_back( RS2_CALIBRATION_FAILED );
                }
                reset();
                switch( _last_status_sent )
                {
                case RS2_CALIBRATION_FAILED:
                case RS2_CALIBRATION_SUCCESSFUL:
                case RS2_CALIBRATION_NOT_NEEDED:
                    // final state -- trigger the next AC
                    _n_cycles = 0;  // now inactive
                    if( !is_on() )
                    {
                        AC_LOG( DEBUG, "Calibration mechanism is not on; not scheduling next calibration" );
                        break;
                    }
                    AC_LOG( DEBUG, "Now inactive" );
                    // Trigger after a set amount of time
                    auto n_seconds = get_trigger_seconds();
                    if( n_seconds.count() )
                        start( n_seconds );
                    else
                        AC_LOG( DEBUG, "RS2_AC_TRIGGER_SECONDS is 0; no time trigger" );
                    // Or after a certain temperature change
                    if( get_temp_diff_trigger() )
                    {
                        if( _last_temp = read_temperature() )
                            _temp_check = retrier::start< temp_check >( *this, std::chrono::seconds( 60 ));
                    }
                    else
                    {
                        AC_LOG( DEBUG, "RS2_AC_TEMP_DIFF is 0; no temperature change trigger" );
                    }
                    break;
                }
            } );
    }

    double ac_trigger::read_temperature()
    {
        // The temperature may depend on streaming?
        auto res = _hwm.send( command{ TEMPERATURES_GET } );
        if( res.size() < sizeof( temperatures ) )  // New temperatures may get added by FW...
        {
            AC_LOG( ERROR,
                    "Failed to get temperatures; result size= "
                        << res.size() << "; expecting at least " << sizeof( temperatures ) );
            return 0;
        }
        auto const & ts = *( reinterpret_cast< temperatures * >( res.data() ) );
        AC_LOG( DEBUG, "LDD temperture is currently " << ts.LDD_temperature << " degrees Celsius" );
        return ts.LDD_temperature;
    }

    void ac_trigger::start( std::chrono::seconds n_seconds )
    {
        if( is_active() )
            throw wrong_api_call_sequence_exception( "AC is already active" );

        if( !n_seconds.count() )
        {
            // Check if we want auto trigger
            if( get_trigger_seconds().count() )
                n_seconds = std::chrono::seconds( 3 );
            else if( get_temp_diff_trigger() &&  ( _last_temp = read_temperature() ))
                _temp_check = retrier::start< temp_check >( *this, std::chrono::seconds( 60 ) );
            else
                return;  // no auto trigger
        }
        _is_on = true;
        if( n_seconds.count() )
        {
            AC_LOG( DEBUG, "Calibration will be triggered in " << n_seconds.count() << " seconds..." );
            // If there's already a trigger, this will simply override it
            _next_trigger = retrier::start( *this, n_seconds, "next calibration" );
        }
    }

    void ac_trigger::stop()
    {
        if( _next_trigger )
        {
            AC_LOG( DEBUG, "Cancelling next calibration" );
            _next_trigger.reset();
        }
        if( is_active() )
        {
            AC_LOG( DEBUG, "Cancelling current calibration" );
            _n_cycles = 0;  // now inactive!
        }
        _temp_check.reset();
        _retrier.reset();
        _recycler.reset();
        _is_on = false;
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
            return 0x5F == mode;
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
            {
                AC_LOG( DEBUG, "frame " << f.get_frame_number() << " is our special frame" );
                _autocal->set_special_frame( f );
            }
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
