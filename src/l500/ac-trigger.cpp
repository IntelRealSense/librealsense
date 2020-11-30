// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "ac-trigger.h"
#include "depth-to-rgb-calibration.h"
#include "l500-device.h"
#include "l500-color.h"
#include "l500-depth.h"
#include "algo/depth-to-rgb-calibration/debug.h"
#include "algo/thermal-loop/l500-thermal-loop.h"
#include "log.h"


#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else 
#include <sys/stat.h>  // mkdir
#include <iostream>  // std::cout
#endif


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

template<>
struct string_to< bool >
{
    static bool convert( std::string const & s )
    {
        if( s.length() == 1 )
        {
            char ch = s.front();
            if( ch == '1' || ch == 'T' )
                return true;
            if( ch == '0' || ch == 'F' )
                return false;
        }
        else
        {
            if( s == "true" || s == "TRUE" || s == "on" || s == "ON" )
                return true;
            if( s == "false" || s == "FALSE" || s == "off" || s == "OFF" )
                return false;
        }
        throw std::invalid_argument( "invalid boolean value '" + s + '\'' );
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


// Returns the current date and time, with the chosen formatting
// See format docs here: https://en.cppreference.com/w/cpp/chrono/c/strftime
// %T = equivalent to "%H:%M:%S"
static std::string now_string( char const * format_string = "%T" )
{
    std::time_t now = std::time( nullptr );
    auto ptm = localtime( &now );
    char buf[256];
    strftime( buf, sizeof( buf ), format_string, ptm );
    return buf;
}



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
        600,  // 10 minutes since last (0 to disable)
        []( int n ) { return n >= 0; } );
    // 0 means turn off auto-trigger
    return std::chrono::seconds( n_seconds );
}


namespace rs2
{
    static std::ostream & operator<<( std::ostream & os, stream_profile const & sp )
    {
        auto spi = sp.get()->profile;
        os << '[';
        if( spi )
        {
            os << rs2_format_to_string( spi->get_format() );
            if( auto vsp = dynamic_cast< const librealsense::video_stream_profile * >( spi ) )
                os << ' ' << vsp->get_width() << 'x' << vsp->get_height();
            os << " " << spi->get_framerate() << "fps";
        }
        os << ']';
        return os;
    }
}


namespace librealsense {
namespace ivcam2 {


    static bool is_auto_trigger_possible()
    {
        if( get_trigger_seconds().count() )
            return true;
        if( get_temp_diff_trigger() )
            return true;
        return false;
    }
    static bool is_auto_trigger_default()
    {
        return env_var< bool >( "RS2_AC_AUTO_TRIGGER",
#ifdef __arm__
                                false
#else
                                false
#endif
                                )
            && is_auto_trigger_possible();
    }

    ac_trigger::enabler_option::enabler_option( std::shared_ptr< ac_trigger > const & autocal )
        : super( option_range{ 0,
                               RS2_CAH_TRIGGER_COUNT - 1,
                               1,
                               is_auto_trigger_default() ? float( RS2_CAH_TRIGGER_AUTO )
                                                         : float( RS2_CAH_TRIGGER_MANUAL ) } )
        , _autocal( autocal )
    {
    }

    ac_trigger::reset_option::reset_option( std::shared_ptr< ac_trigger > const & autocal )
        : bool_option( false )
        , _autocal( autocal )
    {
    }

    void ac_trigger::enabler_option::set( float value )
    {
        if( value == query() )
        {
            return;
        }

        auto ac = _autocal.lock();
        if (!ac)
            throw std::runtime_error( "device no longer exists" );

        if( value != RS2_CAH_TRIGGER_NOW )
        {
            // When auto trigger is on in the environment, we control the timed activation
            // of AC, and do NOT trigger manual calibration
            if( value == RS2_CAH_TRIGGER_AUTO )
            {
                if( ! is_auto_trigger_possible() )
                    throw invalid_value_exception( "auto trigger is disabled in the environment" );
                try
                {
                    if (ac->_dev.get_depth_sensor().is_streaming())
                        ac->_start();

                    super::set(value);
                }
                catch (std::exception const & e)
                {
                    AC_LOG(ERROR, "EXCEPTION caught during start: " << e.what());
                    return;
                }
                // else start() will get called on stream start
            }
            else
            {
                super::set( value );
                ac->stop();
            }
            _record_action( *this );
        }
        else
        {
            // User wants to trigger it RIGHT NOW
            // We don't change the actual control value!
            bool is_depth_streaming(false);

            is_depth_streaming = ac->_dev.get_depth_sensor().is_streaming();

            if( is_depth_streaming )
            {
                AC_LOG( DEBUG, "Triggering manual calibration..." );
                ac->trigger_calibration( calibration_type::MANUAL );
            }
            else
            {
                throw wrong_api_call_sequence_exception( "Cannot trigger calibration: depth sensor is not on!" );
            }
        }
    }

    void ac_trigger::reset_option::set(float value)
    {
        //bool_option::set( value );
        auto ac = _autocal.lock();
        if (!ac)
            throw std::runtime_error( "device no longer exists" );

        // Reset the calibration so we can do it all over again
        if (auto color_sensor = ac->_dev.get_color_sensor())
            color_sensor->reset_calibration();
        ac->_dev.get_depth_sensor().reset_calibration();
        ac->_dev.notify_of_calibration_change(RS2_CALIBRATION_SUCCESSFUL);
        _record_action(*this);
    }


    // Implementation class: starts another thread responsible for sending a retry
    // NOTE that it does this as long as our shared_ptr keeps us alive: once it's gone, if the
    // retry period elapses then nothing will happen!
    class ac_trigger::retrier
    {
        std::weak_ptr<ac_trigger> _ac;
        unsigned _id;
        char const * const _name;

    protected:
        retrier( ac_trigger & ac, char const * name )
            : _ac( ac.shared_from_this() )
            , _name( name ? name : "retrier" )
        {
            static unsigned id = 0;
            _id = ++id;
        }

        unsigned get_id() const { return _id; }
        std::shared_ptr <ac_trigger> get_ac() const { return _ac.lock(); }
        char const * get_name() const { return _name; }

        static std::string _prefix( std::string const & name, unsigned id )
        {
            return to_string() << "... " << now_string() << " " << name << ' ' << id << ": ";
        }
        std::string prefix() const
        {
            return _prefix( get_name(), get_id() );
        }

        virtual void retry(ac_trigger & trigger)
        {
            trigger.trigger_retry();
        }

    public:
        virtual ~retrier()
        {
            AC_LOG( DEBUG, _prefix( '~' + std::string(get_name()), get_id() ));
        }

        template < class T = retrier >
        static std::shared_ptr< T > start( ac_trigger & trigger,
                                           std::chrono::seconds n_seconds,
                                           const char * name = nullptr )
        {
            T * r = new T( trigger, name );
            auto id = r->get_id();
            name = r->get_name();
            AC_LOG( DEBUG, r->prefix() << n_seconds.count() << " seconds starting" );
            auto pr = std::shared_ptr< T >( r );
            std::weak_ptr< T > weak{ pr };
            std::thread( [=]() {
                std::this_thread::sleep_for( n_seconds );
                auto pr = weak.lock();
                if( pr && pr->get_id() == id )
                {
                    try
                    {
                        AC_LOG( DEBUG, _prefix( name, id ) << "triggering" );
                        auto ac = ( (retrier *)pr.get() )->get_ac();
                        if( ac )
                            ( (retrier *)pr.get() )->retry( *ac );
                    }
                    catch( std::exception const & e )
                    {
                        // Unexpected! If we don't handle, we'll crash!
                        AC_LOG( ERROR, "EXCEPTION caught: " << e.what() );
                    }
                }
                else
                    AC_LOG( DEBUG,
                            _prefix( name, id )
                                << n_seconds.count() << " seconds are up; nothing needed" );
            } ).detach();
            return pr;
        }
    };
    class ac_trigger::next_trigger : public ac_trigger::retrier
    {
    public:
        next_trigger( ac_trigger & ac, const char * name )
            : retrier( ac, name ? name : "next trigger" )
        {
        }

    private:
        void retry( ac_trigger & trigger ) override
        {
            // We start another trigger regardless of what happens next; if a calibration actually
            // proceeds, it will be cancelled...
            trigger.schedule_next_calibration();
            try
            {
                trigger.trigger_calibration( calibration_type::AUTO );
            }
            catch( invalid_value_exception const & )
            {
                // Invalid conditions for calibration
                // Error should already have been printed
                // We should already have the next triggers set -- do nothing else
            }
        }
    };
    class ac_trigger::temp_check : public ac_trigger::retrier
    {
    public:
        temp_check( ac_trigger & ac, const char * name )
            : retrier( ac, name ? name : "temp check" )
        {
        }

    private:
        void retry( ac_trigger & trigger ) override
        {
            if( trigger.is_active() )
            {
                AC_LOG( DEBUG, "... already active; ignoring" );
                return;
            }
            // We start another trigger regardless of what happens next; if a calibration actually
            // proceeds, it will be cancelled...
            trigger.schedule_next_temp_trigger();
            auto current_temp = trigger.read_temperature();
            if( current_temp )
            {
                auto d_temp = current_temp - trigger._last_temp;
                if( d_temp >= get_temp_diff_trigger() )
                {
                    try
                    {
                        AC_LOG( DEBUG, "Delta since last successful calibration is " << d_temp << " degrees Celsius; triggering..." );
                        trigger.trigger_calibration( calibration_type::AUTO );
                    }
                    catch( invalid_value_exception const & )
                    {
                        // Invalid conditions for calibration
                        // Error should already have been printed
                        // We should already have the next trigger set -- do nothing else
                    }
                }
                // We do not update the trigger temperature: it is only updated after calibration
            }
        }
    };


    /*
        Temporary (?) class used to direct CAH logs to either console or a special log

        If RS2_DEBUG_DIR is defined in the environment, we try to create a log file in there
        that has the name "<pid>.ac_log".
    */
    class ac_trigger::ac_logger : public rs2_log_callback
    {
        std::ofstream _f_main;
        std::ofstream _f_active;
        std::string _active_dir;
        bool _to_stdout;

    public:
        explicit ac_logger( bool to_stdout = false )
            : _to_stdout( to_stdout )
        {
            using namespace std::chrono;

#ifdef WIN32
            // In the Viewer, a console does not exist (even when run from a console) because
            // it's a WIN32 application (this can be removed in its CMakeLists.txt) -- so we
            // need to create one.
            if( _to_stdout )
            {
                if( AttachConsole( ATTACH_PARENT_PROCESS ) || AllocConsole() )
                    write_out( std::string{} );  // for a newline
            }
#endif

            std::string filename = get_debug_path_base();
            if( ! filename.empty() )
            {
                filename += ".ac_log";

                _f_main.open( filename );
                if( _f_main  &&  _to_stdout )
                    write_out( to_string() << "-D- CAH main log is being written to: " << filename );
            }

            librealsense::log_to_callback( RS2_LOG_SEVERITY_ALL,
                                            { this, []( rs2_log_callback * p ) {} } );

            AC_LOG( DEBUG, "LRS version: " << RS2_API_FULL_VERSION_STR );
        }

        static void add_dir_sep( std::string & path )
        {
#ifdef _WIN32
            char const dir_sep = '\\';
#else
            char const dir_sep = '/';
#endif
            if( ! path.empty() && path.back() != dir_sep )
                path += dir_sep;
        }

        static std::string get_debug_path_base()
        {
            std::string path;

            // If the user has this env var defined, then we write out logs and frames to it
            // NOTE: The var should end with a directory separator \ or /
            auto dir_ = getenv( "RS2_DEBUG_DIR" );
            if( dir_ )
            {
                path = dir_;
                add_dir_sep( path );
                path += now_string( "%y%m%d.%H%M%S" );
            }
            
            return path;
        }

        bool set_active_dir()
        {
            _active_dir = get_debug_path_base();
            if( _active_dir.empty() )
                return false;
            add_dir_sep( _active_dir );

#ifdef _WIN32
            auto status = _mkdir( _active_dir.c_str() );
#else
            auto status = mkdir( _active_dir.c_str(), 0700 ); // 0700 = user r/w/x
#endif

            if( status != 0 )
            {
                AC_LOG( WARNING,
                        "Failed (" << status << ") to create directory for AC frame data in: " << _active_dir );
                _active_dir.clear();
                return false;
            }
            return true;
        }

        std::string const & get_active_dir() const { return _active_dir; }

        void open_active()
        {
            close_active();
            if( ! set_active_dir() )
                return;
            std::string filename = get_active_dir() + "ac.log";
            if( _f_main || _to_stdout )
                AC_LOG( DEBUG, now_string() << "  Active calibration log is being written to: " << filename );
            _f_active.open( filename );
            if( ! _f_active )
                AC_LOG( DEBUG, "             failed!" );
            else if( _to_stdout )
                write_out( to_string() << "-D- CAH active log is being written to: " << filename );
        }

        void close_active()
        {
            if( _f_active )
            {
                _f_active.close();
                _f_active.setstate( std::ios_base::failbit );  // so we don't try to write to it
                _active_dir.clear();
                if( _f_main )
                    AC_LOG( DEBUG, now_string() << "  ... done" );
            }
        }

        void on_log( rs2_log_severity severity, rs2_log_message const & msg ) noexcept override
        {
#ifdef BUILD_EASYLOGGINGPP
            log_message const & wrapper = (log_message const &)(msg);
            char const * raw = wrapper.el_msg.message().c_str();
            if( strncmp( AC_LOG_PREFIX, raw, AC_LOG_PREFIX_LEN ) )
                return;
            std::ostringstream ss;
            ss << "-" << "DIWE"[severity] << "- ";
            ss << (raw + 5);
            std::string text = ss.str();
            if( _to_stdout )
                write_out( text );
            if( _f_active )
                _f_active << text << std::endl;
            else if( _f_main )
                _f_main << text << std::endl;
#endif
        }

        void release() override { delete this; }

    private:
        static void write_out( std::string const & s )
        {
#ifdef WIN32
            HANDLE stdOut = GetStdHandle( STD_OUTPUT_HANDLE );
            if( stdOut != NULL && stdOut != INVALID_HANDLE_VALUE )
            {
                DWORD written = 0;
                WriteFile( stdOut, s.c_str(), (DWORD)s.length(), &written, NULL );
                WriteFile( stdOut, "\n", 1, &written, NULL );
            }
#else
            std::cout << s << std::endl;
#endif
        }
    };


    ac_trigger::ac_trigger( l500_device & dev, std::shared_ptr<hw_monitor> hwm )
        : _hwm( hwm )
        , _dev( dev )
    {
        get_ac_logger();
    }


    ac_trigger::ac_logger & ac_trigger::get_ac_logger()
    {
        static ac_logger one_logger(
            env_var< bool >( "RS2_AC_LOG_TO_STDOUT", false )  // log to stdout
            );
        return one_logger;
    }


    ac_trigger::~ac_trigger() 
    { 
        if( _worker.joinable() )
        {
            _is_processing = false;  // Signal the thread that we want to stop!
            _is_on = false;          // Set auto calibration off so it won't retry.
            _worker.join();
        }
    }


    void ac_trigger::call_back( rs2_calibration_status status )
    {
        _last_status_sent = status;
        for( auto && cb : _callbacks )
            cb( status );
    }


    void ac_trigger::trigger_retry()
    {
        _retrier.reset();
        if( ! is_active() )
        {
            AC_LOG( ERROR, "Retry attempted but we're not active; ignoring" );
            return;
        }
        if( _need_to_wait_for_color_sensor_stability )
        {
            AC_LOG( ERROR, "Failed to receive stable RGB frame; cancelling calibration" );
            cancel_current_calibration();
            return;
        }

        try
        {
            check_conditions();
        }
        catch( invalid_value_exception const & )
        {
            //AC_LOG( ERROR, "Cancelling calibration" );
            cancel_current_calibration();
            return;
        }

        if( _recycler )
        {
            // This is another cycle of AC, after we've woken up from some time after
            // the previous invalid-scene or bad-result...
            _n_retries = 0;
            _recycler.reset();
        }
        else if( ++_n_retries > 4 )
        {
            AC_LOG( ERROR, "Too many retries; aborting" );
            cancel_current_calibration();
            return;
        }

        call_back( RS2_CALIBRATION_RETRY );

        start_color_sensor_if_needed();
        if( _need_to_wait_for_color_sensor_stability )
        {
            AC_LOG( DEBUG, "Waiting for RGB stability before asking for special frame" );
            _retrier = retrier::start( *this, std::chrono::seconds( get_retry_sf_seconds() + 1 ) );
        }
        else
        {
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle " << _n_cycles << " retry " << _n_retries << ")" );
            trigger_special_frame();
        }
    }


    void ac_trigger::trigger_special_frame()
    {
        command cmd{ GET_SPECIAL_FRAME, 0x5F, 1 };  // 5F = SF = Special Frame, for easy recognition
        try
        {
            auto hwm = _hwm.lock();
            if( ! hwm )
            {
                AC_LOG( ERROR, "Hardware monitor is inaccessible - calibration not triggered" );
                return;
            }
            hwm->send( cmd );
        }
        catch( std::exception const & e )
        {
            AC_LOG( ERROR, "EXCEPTION caught: " << e.what() );
        }
        // Start a timer: enable retries if something's wrong with the special frame
        if( is_active() )
            _retrier = retrier::start( *this, std::chrono::seconds( get_retry_sf_seconds() ) );
    }


    void ac_trigger::trigger_calibration( calibration_type type )
    {
        if( is_active() )
        {
            AC_LOG( ERROR, "Failed to trigger calibration: CAH is already active" );
            throw wrong_api_call_sequence_exception( "CAH is already active" );
        }
        if( _retrier.get() || _recycler.get() )
        {
            AC_LOG( ERROR, "Bad inactive state: one of retrier or recycler is set!" );
            throw std::runtime_error( "bad inactive state" );
        }

        _calibration_type = type;
        AC_LOG( DEBUG, "Calibration type is " << (type == calibration_type::MANUAL ? "MANUAL" : "AUTO") );
        
        try
        {
            check_conditions();
        }
        catch( invalid_value_exception const & )
        {
            call_back( RS2_CALIBRATION_BAD_CONDITIONS );
            // Above throws invalid_value_exception, which we want: if calibration is triggered
            // under bad conditions, we want the user to get this!
            throw;
        }

        _n_retries = 0;
        _n_cycles = 1;          // now active
        get_ac_logger().open_active();
        AC_LOG( INFO, "Camera Accuracy Health check is now active (HUM temp is " << _temp << " dec C)" );
        call_back( RS2_CALIBRATION_TRIGGERED );
        _next_trigger.reset();  // don't need a trigger any more
        _temp_check.reset();    // nor a temperature check
        start_color_sensor_if_needed();
        if( _need_to_wait_for_color_sensor_stability )
        {
            AC_LOG( DEBUG, "Waiting for RGB stability before asking for special frame" );
            _retrier = retrier::start( *this, std::chrono::seconds( get_retry_sf_seconds() + 1 ) );
        }
        else
        {
            AC_LOG( DEBUG, "Sending GET_SPECIAL_FRAME (cycle 1)" );
            trigger_special_frame();
        }
    }


    void ac_trigger::start_color_sensor_if_needed()
    {
        // With AC, we need a color sensor even when the user has not asked for one --
        // otherwise we risk misalignment over time. We turn it on automatically!
        try
        {
            auto color_sensor = _dev.get_color_sensor();
            if( ! color_sensor )
            {
                AC_LOG( ERROR, "No color sensor in device; cannot run AC?!" );
                return;
            }

            auto & depth_sensor = _dev.get_depth_sensor();
            auto rgb_profile = depth_sensor.is_color_sensor_needed();
            if( ! rgb_profile )
                return;  // error should have already been printed
            //AC_LOG( DEBUG, "Picked profile: " << *rgb_profile );

            // If we just started the sensor, especially for AC, then we cannot simply take
            // the first RGB frame we get: for various reasons, including auto exposure, the
            // first frames we receive cannot be used (there's a "fade-in" effect). So we
            // only enable frame capture after some time has passed (see set_color_frame):
            _rgb_sensor_start = std::chrono::high_resolution_clock::now();
            _need_to_wait_for_color_sensor_stability
                = color_sensor->start_stream_for_calibration( { rgb_profile } );
        }
        catch( std::exception const & e )
        {
            AC_LOG( ERROR, "EXCEPTION caught: " << e.what() );
        }
    }


    void ac_trigger::stop_color_sensor_if_started()
    {
        _need_to_wait_for_color_sensor_stability = false;  // jic

        // By using a thread we protect a case that tries to close a sensor from it's processing block callback and creates a deadlock.
        std::thread([&]()
        {
            try
            {
                auto & color_sensor = *_dev.get_color_sensor();
                color_sensor.stop_stream_for_calibration();
            }
            catch (std::exception& ex)
            {
                AC_LOG(ERROR, "caught exception in closing color sensor: " << ex.what());
            }
            catch (...)
            {
                AC_LOG(ERROR, "unknown exception in closing color sensor");
            }
        }).detach();
        
    }


    void ac_trigger::set_special_frame( rs2::frameset const & fs )
    {
        if( ! is_active() )
        {
            AC_LOG( ERROR, "Special frame received while is_active() is false" );
            return;
        }

        // Notify of the special frame -- mostly for validation team so they know to expect a frame
        // drop...
        call_back( RS2_CALIBRATION_SPECIAL_FRAME );

        if( _is_processing )
        {
            AC_LOG( ERROR, "already processing; ignoring special frame!" );
            return;
        }
        auto irf = fs.get_infrared_frame();
        if( ! irf )
        {
            AC_LOG( ERROR, "no IR frame found; ignoring special frame!" );
            //call_back( RS2_CALIBRATION_FAILED );
            return;
        }
        auto df = fs.get_depth_frame();
        if( ! df )
        {
            AC_LOG( ERROR, "no depth frame found; ignoring special frame!" );
            //call_back( RS2_CALIBRATION_FAILED );
            return;
        }

        _retrier.reset();  // No need to activate a retry if the following takes a bit of time!

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
        if( ! is_active()  ||  _is_processing )
            // No error message -- we expect to get new color frames while processing...
            return;
        if( _need_to_wait_for_color_sensor_stability )
        {
            // Wait at least a second before we deem the sensor stable enough
            auto time = std::chrono::high_resolution_clock::now() - _rgb_sensor_start;
            if( time < std::chrono::milliseconds( 1000 ) )
                return;

            auto number = f.get_frame_number();
            AC_LOG( DEBUG, "RGB frame #" << number << " is our first stable frame" );
            if( f.supports_frame_metadata( RS2_FRAME_METADATA_ACTUAL_EXPOSURE ) )
            {
                AC_LOG( DEBUG,
                        "    actual exposure= "
                            << f.get_frame_metadata( RS2_FRAME_METADATA_ACTUAL_EXPOSURE ) );
                AC_LOG( DEBUG,
                        "    backlight compensation= "
                            << f.get_frame_metadata( RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION ) );
                AC_LOG( DEBUG,
                        "    brightness= "
                            << f.get_frame_metadata( RS2_FRAME_METADATA_BRIGHTNESS ) );
                AC_LOG( DEBUG,
                        "    contrast= "
                            << f.get_frame_metadata( RS2_FRAME_METADATA_CONTRAST ) );
            }
            _need_to_wait_for_color_sensor_stability = false;
            trigger_special_frame();
        }

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
        if( ! _pcf )
        {
            AC_LOG( DEBUG, "no prev color frame received" );
            return false;
        }
        return true;
    }

     rs2_intrinsics read_intrinsics_from_camera( l500_device & dev,
                                                      const rs2::stream_profile & profile )
    {
        auto vp = profile.as< rs2::video_stream_profile >(); 
        return dev.get_color_sensor()->get_raw_intrinsics(vp.width(), vp.height() );
    }

    void ac_trigger::run_algo()
    {
        AC_LOG( DEBUG,
                "Starting processing:"
                    << "  color #" << _cf.get_frame_number() << " " << _cf.get_profile()
                    << "  depth #" << _sf.get_frame_number() << ' ' << _sf.get_profile() );
        stop_color_sensor_if_started();
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

                    // We have to read the FW registers at the time of the special frame, but we
                    // cannot do it from set_special_frame() because it takes time and we should not
                    // hold up the thread that the frame callbacks are on!
                    float dsm_x_scale, dsm_y_scale, dsm_x_offset, dsm_y_offset;
                    algo::depth_to_rgb_calibration::algo_calibration_info cal_info;
                    algo::thermal_loop::l500::thermal_calibration_table thermal_table;
                    {
                        auto hwm = _hwm.lock();
                        if( ! hwm )
                            throw std::runtime_error( "HW monitor is inaccessible - stopping algo" );

                        ivcam2::read_fw_register( *hwm, &dsm_x_scale, 0xfffe3844 );
                        ivcam2::read_fw_register( *hwm, &dsm_y_scale, 0xfffe3830 );
                        ivcam2::read_fw_register( *hwm, &dsm_x_offset, 0xfffe3840 );
                        ivcam2::read_fw_register( *hwm, &dsm_y_offset, 0xfffe382c );

                        ivcam2::read_fw_table( *hwm, cal_info.table_id, &cal_info );

                        // If the above throw (and they can!) then we catch below and stop...

                        try
                        {
                            thermal_table = _dev.get_color_sensor()->get_thermal_table();
                        }
                        catch( std::exception const & e )
                        {
                            AC_LOG( WARNING, std::string( to_string() << "Disabling thermal correction: " << e.what() << " [NO-THERMAL]" ));
                        }
                    }

                    AC_LOG( DEBUG,
                            "dsm registers=  x[" << AC_F_PREC << dsm_x_scale << ' ' << dsm_y_scale
                                                 << "]  +[" << dsm_x_offset << ' ' << dsm_y_offset
                                                 << "]" );
                    algo::depth_to_rgb_calibration::algo_calibration_registers cal_regs;
                    cal_regs.EXTLdsmXscale  = dsm_x_scale;
                    cal_regs.EXTLdsmYscale  = dsm_y_scale;
                    cal_regs.EXTLdsmXoffset = dsm_x_offset;
                    cal_regs.EXTLdsmYoffset = dsm_y_offset;

                    auto df = _sf.get_depth_frame();
                    auto irf = _sf.get_infrared_frame();

                    auto should_continue = [&]()
                    {
                        if( ! is_processing() )
                        {
                            AC_LOG( DEBUG, "Stopping algo: not processing any more" );
                            throw std::runtime_error( "stopping algo: not processing any more" );
                        }
                    };
                    algo::depth_to_rgb_calibration::optimizer::settings settings;
                    settings.is_manual_trigger = _calibration_type == calibration_type::MANUAL;
                    settings.hum_temp = _temp;
                    settings.digital_gain = _digital_gain;
                    settings.receiver_gain = _receiver_gain;
                    depth_to_rgb_calibration algo(
                        settings,
                        df,
                        irf,
                        _cf,
                        _pcf,
                        _last_yuy_data,
                        cal_info,
                        cal_regs,
                        read_intrinsics_from_camera( _dev, _cf.get_profile() ),
                        thermal_table,
                        should_continue );

                    std::string debug_dir = get_ac_logger().get_active_dir();
                    if( ! debug_dir.empty() )
                        algo.write_data_to( debug_dir );

                    _from_profile = algo.get_from_profile();
                    _to_profile = algo.get_to_profile();

                    rs2_calibration_status status = RS2_CALIBRATION_FAILED;  // assume fail until we get a success

                    if( is_processing() )  // check if the device still exists
                    {
                        status = algo.optimize(
                            [this]( rs2_calibration_status status ) { call_back( status ); });
                    }

                    // It's possible that, while algo was working, stop() was called. In this case,
                    // we have to make sure that we notify of failure:
                    if( ! is_processing() )
                    {
                        AC_LOG( DEBUG, "Algo finished (with " << status << "), but stop() was detected; notifying of failure..." );
                        status = RS2_CALIBRATION_FAILED;
                    }

                    switch( status )
                    {
                    case RS2_CALIBRATION_SUCCESSFUL:
                        _extr = algo.get_extrinsics();
                        _raw_intr = algo.get_raw_intrinsics();
                        _thermal_intr = algo.get_thermal_intrinsics();
                        _dsm_params = algo.get_dsm_params();
                        call_back( status );  // if this throws, we don't want to do the below:
                        _last_temp = _temp;
                        _last_yuy_data = std::move( algo.get_last_successful_frame_data() );
                        break;
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
                            AC_LOG( ERROR, "Too many retry cycles; quitting" );
                            call_back( RS2_CALIBRATION_FAILED );
                        }
                        else
                        {
                            AC_LOG( DEBUG, "Waiting for retry cycle " << _n_cycles << " ..." );
                            bool const is_manual = _calibration_type == calibration_type::MANUAL;
                            int n_seconds
                                = env_var< int >( is_manual ? "RS2_AC_INVALID_RETRY_SECONDS_MANUAL"
                                                            : "RS2_AC_INVALID_RETRY_SECONDS_AUTO",
                                                  is_manual ? 2 : 60,
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
                            "Unexpected status '" << status << "' received from CAH algo; stopping!" );
                        call_back( RS2_CALIBRATION_FAILED );
                        break;
                    }
                }
                catch( std::exception & e )
                {
                    AC_LOG( ERROR, "EXCEPTION in calibration algo: " << e.what() );
                    try
                    {
                        call_back( RS2_CALIBRATION_FAILED );
                    }
                    catch( std::exception & e )
                    {
                        AC_LOG( ERROR, "EXCEPTION in FAILED callback: " << e.what() );
                    }
                    catch( ... )
                    {
                        AC_LOG( ERROR, "EXCEPTION in FAILED callback!" );
                    }
                }
                catch( ... )
                {
                    AC_LOG( ERROR, "Unknown EXCEPTION in calibration algo!!!" );
                    try
                    {
                        call_back( RS2_CALIBRATION_FAILED );
                    }
                    catch( std::exception & e )
                    {
                        AC_LOG( ERROR, "EXCEPTION in FAILED callback: " << e.what() );
                    }
                    catch( ... )
                    {
                        AC_LOG( ERROR, "EXCEPTION in FAILED callback!" );
                    }
                }
                _is_processing = false;  // to avoid debug msg in reset()
                reset();
                switch( _last_status_sent )
                {
                case RS2_CALIBRATION_FAILED:
                case RS2_CALIBRATION_SUCCESSFUL:
                case RS2_CALIBRATION_NOT_NEEDED:
                    calibration_is_done();
                    break;
                }
            } );
    }


    void ac_trigger::cancel_current_calibration()
    {
        if( ! is_active() )
            return;
        if( is_processing() )
        {
            reset();
            // Wait until we're out of run_algo() -- until then, we're active!
        }
        else
        {
            stop_color_sensor_if_started();
            call_back( RS2_CALIBRATION_FAILED );
            reset();
            _retrier.reset();
            _recycler.reset();
            calibration_is_done();
        }
    }


    void ac_trigger::calibration_is_done()
    {
        if( is_active() )
        {
            // We get here when we've reached some final state (failed/successful)
            if( _last_status_sent != RS2_CALIBRATION_SUCCESSFUL )
                AC_LOG( WARNING, "Camera Accuracy Health has finished unsuccessfully" );
            else
                AC_LOG( INFO, "Camera Accuracy Health has finished" );
            set_not_active();
        }

        schedule_next_calibration();
    }


    void ac_trigger::set_not_active()
    {
        _n_cycles = 0;
        get_ac_logger().close_active();
    }


    void ac_trigger::schedule_next_calibration()
    {
        // Trigger the next CAH -- but only if we're "on", meaning this wasn't a manual calibration
        if( ! auto_calibration_is_on() )
        {
            AC_LOG( DEBUG, "Calibration mechanism is not on; not scheduling next calibration" );
            return;
        }

        schedule_next_time_trigger();
        schedule_next_temp_trigger();
    }

    void ac_trigger::schedule_next_time_trigger( std::chrono::seconds n_seconds )
    {
        if( ! n_seconds.count() )
        {
            n_seconds = get_trigger_seconds();
            if (!n_seconds.count())
            {
                AC_LOG(DEBUG, "RS2_AC_TRIGGER_SECONDS is 0; no time trigger");
                return;
            }
        }

        // If there's already a trigger, this will simply override it
        _next_trigger = retrier::start< next_trigger >( *this, n_seconds );
    }

    void ac_trigger::schedule_next_temp_trigger()
    {
        // If no _last_temp --> first calibration! We want to keep checking every minute
        // until temperature is within proper conditions...
        if( get_temp_diff_trigger()  ||  ! _last_temp )
        {
            //AC_LOG( DEBUG, "Last HUM temperature= " << _last_temp );
            _temp_check = retrier::start< temp_check >( *this, std::chrono::seconds( 60 ) );
        }
        else
        {
            AC_LOG( DEBUG, "RS2_AC_TEMP_DIFF is 0; no temperature change trigger" );
        }
    }


    double ac_trigger::read_temperature()
    {
        return _dev.get_temperatures().HUM_temperature;
    }


    void ac_trigger::check_conditions()
    {
        // Make sure we're still streaming
        bool is_streaming = false;
        try
        {
            is_streaming = _dev.get_depth_sensor().is_streaming();
        }
        catch( std::exception const & e )
        {
            AC_LOG( ERROR, "EXCEPTION caught: " << e.what() );
        }
        if( ! is_streaming )
        {
            AC_LOG( ERROR, "Not streaming; stopping" );
            stop();
            throw wrong_api_call_sequence_exception( "not streaming" );
        }

        std::string invalid_reason;

        auto alt_ir_is_on = false;
        try
        {
            auto & depth_sensor = _dev.get_depth_sensor();
            alt_ir_is_on = depth_sensor.supports_option( RS2_OPTION_ALTERNATE_IR )
                        && depth_sensor.get_option( RS2_OPTION_ALTERNATE_IR ).query() == 1.f;
        }
        catch( std::exception const & e )
        {
            AC_LOG( DEBUG,
                    std::string( to_string() << "Error while checking alternate IR option: " << e.what() ) );
        }
        if( alt_ir_is_on )
        {
            if( ! invalid_reason.empty() )
                invalid_reason += ", ";
            invalid_reason += to_string() << "alternate IR is on";
        }

        _temp = read_temperature();

        auto thermal_table_valid = true;
        try
        {
            _dev.get_color_sensor()->get_thermal_table();
        }
        catch( std::exception const & e )
        {
            AC_LOG( DEBUG, std::string( to_string() << "Disabling thermal correction: " << e.what() << " [NO-THERMAL]" ));
            thermal_table_valid = false;
        }

        if( ! thermal_table_valid )
        {
            if( _temp < 32. )
            {
                // If from some reason there is no thermal_table or its not valid
                // Temperature must be within range or algo may not work right
                if( ! invalid_reason.empty() )
                    invalid_reason += ", ";
                invalid_reason += to_string() << "temperature (" << _temp << ") too low (<32)";
            }
            else if( _temp > 46. )
            {
                if( ! invalid_reason.empty() )
                    invalid_reason += ", ";
                invalid_reason += to_string() << "temperature (" << _temp << ") too high (>46)";
            }
        }
       

        // Algo was written with specific receiver gain (APD) in mind, depending on
        // the FW preset (ambient light)
        auto & depth_sensor = _dev.get_depth_sensor();
        auto & digital_gain = depth_sensor.get_option(RS2_OPTION_DIGITAL_GAIN);
        float raw_digital_gain = digital_gain.query();
        auto & apd = depth_sensor.get_option( RS2_OPTION_AVALANCHE_PHOTO_DIODE );
        float raw_apd = apd.query();
        _receiver_gain = int( raw_apd );
        _digital_gain = ( rs2_digital_gain ) int(raw_digital_gain);
        switch(_digital_gain)
        {
        case RS2_DIGITAL_GAIN_LOW:
            if( _receiver_gain != 18 )
            {
                if( ! invalid_reason.empty() )
                    invalid_reason += ", ";
                invalid_reason += to_string()
                               << "receiver gain(" << raw_apd << ") of 18 is expected with low digital gain(SHORT)";
            }
            break;

        case RS2_DIGITAL_GAIN_HIGH:  
            if( _receiver_gain != 9 )
            {
                if( ! invalid_reason.empty() )
                    invalid_reason += ", ";
                invalid_reason += to_string()
                               << "receiver gain(" << raw_apd << ") of 9 is expected with high digital gain(LONG)";
            }
            break;

        default:
            if( ! invalid_reason.empty() )
                invalid_reason += ", ";
            invalid_reason += to_string() << "invalid (" << raw_digital_gain << ") digital gain preset";
            break;
        }

        if( ! invalid_reason.empty() )
        {
            // This can and will happen every minute (from the temp check), therefore not an error...
            AC_LOG( DEBUG, "Invalid conditions for CAH: " << invalid_reason );
            if( ! env_var< bool >( "RS2_AC_DISABLE_CONDITIONS", false ) )
                throw invalid_value_exception( invalid_reason );
            AC_LOG( DEBUG, "RS2_AC_DISABLE_CONDITIONS is on; continuing anyway" );
        }
    }

    void ac_trigger::start()
    {
        try
        {
            option & o
                = _dev.get_depth_sensor().get_option( RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH );
            if( o.query() != float( RS2_CAH_TRIGGER_AUTO ) )
            {
                // auto trigger is turned off
                AC_LOG( DEBUG, "Turned off -- no trigger set" );
                return;
            }
        }
        catch( std::exception const & e )
        {
            AC_LOG( ERROR, "EXCEPTION caught in access to device: " << e.what() );
            return;
        }

        _start();
    }

    void ac_trigger::_start()
    {
        if( auto_calibration_is_on() )
            throw wrong_api_call_sequence_exception( "CAH is already active" );

        if( ! is_auto_trigger_possible() )
        {
            AC_LOG( DEBUG, "Auto trigger is disabled in environment" );
            return;  // no auto trigger
        }

        _is_on = true;

        // If we are already active then we don't need to do anything: another calibration will be
        // triggered automatically at the end of the current one 
        if( is_active() )
        {
            return;
        }

        // Note: we arbitrarily choose the time before CAH starts at 10 second -- enough time to
        // make sure the user isn't going to fiddle with color sensor activity too much, because
        // if color is off then CAH will automatically turn it on!
        schedule_next_time_trigger( std::chrono::seconds( 10 ) );
    }

    void ac_trigger::stop()
    {
        _is_on = false;
        if (is_active())
        {
            cancel_current_calibration();
        }
        else
        {
            if (_next_trigger)
            {
                AC_LOG(DEBUG, "Cancelling next time trigger");
                _next_trigger.reset();
            }

            if (_temp_check)
            {
                AC_LOG(DEBUG, "Cancelling next temp trigger");
                _temp_check.reset();
            }
        }
    }

    void ac_trigger::reset()
    {
        _sf = rs2::frame{};
        _cf = rs2::frame{};;
        _pcf = rs2::frame{};

        _need_to_wait_for_color_sensor_stability = false;
        if( _is_processing )
        {
            _is_processing = false;
            AC_LOG( DEBUG, "Algo is processing; signalling stop" );
        }
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
        // CAH can be triggered manually, too, so we do NOT check whether the option is on!
        auto fs = f.as< rs2::frameset >();
        auto autocal = _autocal.lock();
        if( fs )
        {
            auto df = fs.get_depth_frame();
            if( autocal && autocal->is_expecting_special_frame() && is_special_frame( df ) )
            {
                AC_LOG( DEBUG, "Depth frame #" << f.get_frame_number() << " is our special frame" );
                autocal->set_special_frame( f );
            }
            // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
            return rs2::frame{};
        }

        if( autocal && autocal->is_expecting_special_frame() && is_special_frame( f.as< rs2::depth_frame >() ) )
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
        // CAH can be triggered manually, too, so we do NOT check whether the option is on!

        // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
        if( f.is< rs2::frameset >() )
            return rs2::frame{};

        auto autocal = _autocal.lock();
        if( autocal )
            // We record each and every color frame
            autocal->set_color_frame( f );

        // Return the frame as is!
        return f;
    }


    bool ac_trigger::color_processing_block::should_process( const rs2::frame& frame )
    {
        return true;
    }


}  // namespace ivcam2
}  // namespace librealsense
