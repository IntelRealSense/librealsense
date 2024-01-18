// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "unit-tests-common.h"


std::vector< profile >
configure_all_supported_streams( rs2::sensor & sensor, int width, int height, int fps )
{
    std::vector<profile> all_profiles =
    {
        { RS2_STREAM_DEPTH,     RS2_FORMAT_Z16,           width, height,    0, fps},
        { RS2_STREAM_COLOR,     RS2_FORMAT_RGB8,          width, height,    0, fps},
        { RS2_STREAM_INFRARED,  RS2_FORMAT_Y8,            width, height,    1, fps},
        { RS2_STREAM_INFRARED,  RS2_FORMAT_Y8,            width, height,    2, fps},
        { RS2_STREAM_INFRARED,  RS2_FORMAT_Y8,            width, height,    0, fps},
        { RS2_STREAM_CONFIDENCE,RS2_FORMAT_RAW8,          width, height,    0, fps},
        { RS2_STREAM_FISHEYE,   RS2_FORMAT_RAW8,          width, height,    0, fps},
        { RS2_STREAM_ACCEL,     RS2_FORMAT_MOTION_XYZ32F,   1,      1,      0, 200},
        { RS2_STREAM_GYRO,      RS2_FORMAT_MOTION_XYZ32F,   1,      1,      0, 200},
    };

    std::vector< profile > profiles;
    std::vector< rs2::stream_profile > modes;
    auto all_modes = sensor.get_stream_profiles();

    for( auto profile : all_profiles )
    {
        if( std::find_if( all_modes.begin(),
                          all_modes.end(),
                          [&]( rs2::stream_profile p ) {
                              if( auto video = p.as< rs2::video_stream_profile >() )
                              {
                                  if( p.fps() == profile.fps && p.stream_index() == profile.index
                                      && p.stream_type() == profile.stream && p.format() == profile.format
                                      && video.width() == profile.width && video.height() == profile.height )
                                  {
                                      modes.push_back( p );
                                      return true;
                                  }
                              }
                              else
                              {
                                  if( auto motion = p.as< rs2::motion_stream_profile >() )
                                  {
                                      if( ( ( profile.fps / ( p.fps() + 1 ) ) <= 2.f )
                                          &&  // Approximate IMU rates. No need for being exact
                                          p.stream_index() == profile.index && p.stream_type() == profile.stream
                                          && p.format() == profile.format )
                                      {
                                          modes.push_back( p );
                                          return true;
                                      }
                                  }
                                  else
                                      return false;
                              }

                              return false;
                          } )
            != all_modes.end() )
        {
            profiles.push_back( profile );
        }
    }
    if( modes.size() > 0 )
        REQUIRE_NOTHROW( sensor.open( modes ) );
    return profiles;
}


std::pair< std::vector< rs2::sensor >, std::vector< profile > >
configure_all_supported_streams( rs2::device & dev, int width, int height, int fps )
{
    std::vector< profile > profiles;
    std::vector< rs2::sensor > sensors;
    auto sens = dev.query_sensors();
    for( auto s : sens )
    {
        auto res = configure_all_supported_streams( s, width, height, fps );
        profiles.insert( profiles.end(), res.begin(), res.end() );
        if( res.size() > 0 )
        {
            sensors.push_back( s );
        }
    }

    return { sensors, profiles };
}


std::string space_to_underscore( const std::string & text )
{
    const std::string from = " ";
    const std::string to = "__";
    auto temp = text;
    size_t start_pos = 0;
    while( ( start_pos = temp.find( from, start_pos ) ) != std::string::npos )
    {
        temp.replace( start_pos, from.size(), to );
        start_pos += to.size();
    }
    return temp;
}


//inline long long current_time()
//{
//    return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 10000);
//}

//// disable in one place options that are sensitive to frame content
//// this is done to make sure unit-tests are deterministic
void disable_sensitive_options_for( rs2::sensor & sen )
{
    if( sen.supports( RS2_OPTION_ERROR_POLLING_ENABLED ) )
        REQUIRE_NOTHROW( sen.set_option( RS2_OPTION_ERROR_POLLING_ENABLED, 0 ) );

    if( sen.supports( RS2_OPTION_ENABLE_AUTO_EXPOSURE ) )
        REQUIRE_NOTHROW( sen.set_option( RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0 ) );

    if( sen.supports( RS2_OPTION_GLOBAL_TIME_ENABLED ) )
        REQUIRE_NOTHROW( sen.set_option( RS2_OPTION_GLOBAL_TIME_ENABLED, 0 ) );

    if( sen.supports( RS2_OPTION_EXPOSURE ) )
    {
        rs2::option_range range;
        REQUIRE_NOTHROW(
            range = sen.get_option_range( RS2_OPTION_EXPOSURE ) );  // TODO: fails sometimes with "Element Not Found!"
        // float val = (range.min + (range.def-range.min)/10.f); // TODO - generate new records using the modified
        // exposure

        REQUIRE_NOTHROW( sen.set_option( RS2_OPTION_EXPOSURE, range.def ) );
    }
}


void disable_sensitive_options_for( rs2::device & dev )
{
    for( auto && s : dev.query_sensors() )
        disable_sensitive_options_for( s );
}


bool wait_for_reset( std::function< bool( void ) > func, std::shared_ptr< rs2::device > dev )
{
    if( func() )
        return true;

    WARN( "Reset workaround" );

    try
    {
        dev->hardware_reset();
    }
    catch( ... )
    {
    }
    return func();
}


bool is_usb3( const rs2::device & dev )
{
    bool usb3_device = true;
    try
    {
        std::string usb_type( dev.get_info( RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR ) );
        // Device types "3.x" and also "" (for legacy playback records) will be recognized as USB3
        usb3_device = ( std::string::npos == usb_type.find( "2." ) );
    }
    catch( ... )  // In case the feature not provided assume USB3 device
    {
    }

    return usb3_device;
}


dev_type get_PID( rs2::device & dev )
{
    dev_type designator{ "", true };
    std::string usb_type{};
    bool usb_descriptor = false;

    REQUIRE_NOTHROW( designator.first = dev.get_info( RS2_CAMERA_INFO_PRODUCT_ID ) );
    REQUIRE_NOTHROW( usb_descriptor = dev.supports( RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR ) );
    if( usb_descriptor )
    {
        REQUIRE_NOTHROW( usb_type = dev.get_info( RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR ) );
        designator.second = ( std::string::npos == usb_type.find( "2." ) );
    }

    return designator;
}


void command_line_params::init( int argc, char const * const * const argv )
{
    rs2::log_to_file( RS2_LOG_SEVERITY_DEBUG );
}


static bool g_found_any_section = false;


bool found_any_section()
{
    return g_found_any_section;
}


rs2::context make_context( const char * /*id*/ )
{
    rs2::context ctx( rs2::context::uninitialized );
    try
    {
        ctx = rs2::context( "{\"dds\":false}" );
        g_found_any_section = true;
    }
    catch( ... )
    {
        std::cerr << "UNKNOWN EXCEPTiON in make_context!" << std::endl;
    }
    return ctx;
}


// Require that matrix is an orthonormal 3x3 matrix
void require_rotation_matrix(const float(&matrix)[9])
{
    const float row0[] = { matrix[0], matrix[3], matrix[6] };
    const float row1[] = { matrix[1], matrix[4], matrix[7] };
    const float row2[] = { matrix[2], matrix[5], matrix[8] };
    CAPTURE( full_precision( row0[0] ));
    CAPTURE( full_precision( row0[1] ));
    CAPTURE( full_precision( row0[2] ));
    CAPTURE( full_precision( row1[0] ));
    CAPTURE( full_precision( row1[1] ));
    CAPTURE( full_precision( row1[2] ));
    CAPTURE( full_precision( row2[0] ));
    CAPTURE( full_precision( row2[1] ));
    CAPTURE( full_precision( row2[2] ));
    CHECK(dot_product(row0, row0) == approx(1.f));
    CAPTURE( full_precision( dot_product( row1, row1 )));
    CHECK( dot_product( row1, row1 ) == approx( 1.f ) );     // this line is problematic, and needs higher epsilon!!
    CHECK_THAT(dot_product(row1, row1), approx_equals(1.f));
    CHECK(dot_product(row2, row2) == approx(1.f));
    CHECK(dot_product(row0, row1) == approx(0.f));
    CHECK(dot_product(row1, row2) == approx(0.f));
    CHECK(dot_product(row2, row0) == approx(0.f));
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
}

// Require that matrix is exactly the identity matrix
void require_identity_matrix( const float ( &matrix )[9] )
{
    static const float identity_matrix_3x3[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    for( int i = 0; i < 9; ++i )
        REQUIRE( matrix[i] == approx( identity_matrix_3x3[i] ) );
}

//inline void test_wait_for_frames(rs2_device * device, std::initializer_list<stream_mode>& modes, std::map<rs2_stream, test_duration>& duration_per_stream)
//{
//    rs2_start_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 1 );
//
//    rs2_wait_for_frames(device, require_no_error());
//
//    int lowest_fps_mode = std::numeric_limits<int>::max();
//    for(auto & mode : modes)
//    {
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//        REQUIRE( rs2_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
//        REQUIRE( rs2_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
//        if (mode.framerate < lowest_fps_mode) lowest_fps_mode = mode.framerate;
//    }
//
//    std::vector<unsigned long long> last_frame_number;
//    std::vector<unsigned long long> number_of_frames;
//    last_frame_number.resize(RS2_STREAM_COUNT);
//    number_of_frames.resize(RS2_STREAM_COUNT);
//
//    for (auto& elem : modes)
//    {
//        number_of_frames[elem.stream] = 0;
//        last_frame_number[elem.stream] = 0;
//    }
//
//    const auto actual_frames_to_receive = std::min(max_frames_to_receive, (uint32_t)(max_capture_time_sec* lowest_fps_mode));
//    const auto first_frame_to_capture = (actual_frames_to_receive*0.1);   // Skip the first 10% of frames
//    const auto frames_to_capture = (actual_frames_to_receive - first_frame_to_capture);
//
//    for (auto i = 0u; i< actual_frames_to_receive; ++i)
//    {
//        rs2_wait_for_frames(device, require_no_error());
//
//        if (i < first_frame_to_capture) continue;       // Skip some frames at the beginning to stabilize the stream output
//
//        for (auto & mode : modes)
//        {
//            if (rs2_get_frame_timestamp(device, mode.stream, require_no_error()) > 0)
//            {
//                REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1);
//                REQUIRE( rs2_get_frame_data(device, mode.stream, require_no_error()) != nullptr);
//                REQUIRE( rs2_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0);
//
//                auto frame_number = rs2_get_frame_number(device, mode.stream, require_no_error());
//                if (!duration_per_stream[mode.stream].is_end_time_initialized && last_frame_number[mode.stream] != frame_number)
//                {
//                    last_frame_number[mode.stream] = frame_number;
//                    ++number_of_frames[mode.stream];
//                }
//
//                if (!duration_per_stream[mode.stream].is_start_time_initialized && number_of_frames[mode.stream] >= 1)
//                {
//                    duration_per_stream[mode.stream].start_time = std::chrono::high_resolution_clock::now();
//                    duration_per_stream[mode.stream].is_start_time_initialized = true;
//                }
//
//                if (!duration_per_stream[mode.stream].is_end_time_initialized && (number_of_frames[mode.stream] > (0.9 * frames_to_capture))) // Requires additional work for streams with different fps
//                {
//                    duration_per_stream[mode.stream].end_time = std::chrono::high_resolution_clock::now();
//                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(duration_per_stream[mode.stream].end_time - duration_per_stream[mode.stream].start_time).count();
//                    auto fps = ((double)number_of_frames[mode.stream] / duration) * 1000.;
//                    //check_fps(fps, mode.framerate);   // requires additional work to configure UVC controls in order to achieve the required fps
//                    duration_per_stream[mode.stream].is_end_time_initialized = true;
//                }
//            }
//        }
//    }
//
//    rs2_stop_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 0 );
//}


static std::mutex m;
static std::mutex cb_mtx;
static std::condition_variable cv;
static std::atomic<bool> stop_streaming;
static int done;


void frame_callback( rs2::device & dev, rs2::frame frame, void * user )
{
    std::lock_guard< std::mutex > lock( cb_mtx );

    if( stop_streaming || ( frame.get_timestamp() == 0 ) )
        return;

    auto data = (user_data *)user;
    bool stop = true;

    auto stream_type = frame.get_profile().stream_type();

    for( auto & elem : data->number_of_frames_per_stream )
    {
        if( elem.second < data->duration_per_stream[stream_type].actual_frames_to_receive )
        {
            stop = false;
            break;
        }
    }


    if( stop )
    {
        stop_streaming = true;
        {
            std::lock_guard< std::mutex > lk( m );
            done = true;
        }
        cv.notify_one();
        return;
    }

    if( data->duration_per_stream[stream_type].is_end_time_initialized )
        return;

    unsigned num_of_frames = 0;
    num_of_frames = ( ++data->number_of_frames_per_stream[stream_type] );

    if( num_of_frames >= data->duration_per_stream[stream_type].actual_frames_to_receive )
    {
        if( ! data->duration_per_stream[stream_type].is_end_time_initialized )
        {
            data->duration_per_stream[stream_type].end_time = std::chrono::high_resolution_clock::now();
            data->duration_per_stream[stream_type].is_end_time_initialized = true;
        }

        return;
    }

    if( ( num_of_frames == data->duration_per_stream[stream_type].first_frame_to_capture )
        && ( ! data->duration_per_stream[stream_type]
                   .is_start_time_initialized ) )  // Skip some frames at the beginning to stabilize the stream output
    {
        data->duration_per_stream[stream_type].start_time = std::chrono::high_resolution_clock::now();
        data->duration_per_stream[stream_type].is_start_time_initialized = true;
    }

    REQUIRE( frame.get_data() != nullptr );
    REQUIRE( frame.get_timestamp() >= 0 );
}

//inline void test_frame_callback(device &device, std::initializer_list<stream_profile>& modes, std::map<rs2_stream, test_duration>& duration_per_stream)
//{
//    done = false;
//    stop_streaming = false;
//    user_data data;
//    data.duration_per_stream = duration_per_stream;
//
//    for(auto & mode : modes)
//    {
//        data.number_of_frames_per_stream[mode.stream] = 0;
//        data.duration_per_stream[mode.stream].is_start_time_initialized = false;
//        data.duration_per_stream[mode.stream].is_end_time_initialized = false;
//        data.duration_per_stream[mode.stream].actual_frames_to_receive = std::min(max_frames_to_receive, (uint32_t)(max_capture_time_sec* mode.fps));
//        data.duration_per_stream[mode.stream].first_frame_to_capture = (uint32_t)(data.duration_per_stream[mode.stream].actual_frames_to_receive*0.1);   // Skip the first 10% of frames
//        data.duration_per_stream[mode.stream].frames_to_capture = data.duration_per_stream[mode.stream].actual_frames_to_receive - data.duration_per_stream[mode.stream].first_frame_to_capture;
//        // device doesn't know which subdevice is providing which stream. could loop over all subdevices? no, because subdevices don't expose this information
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//        rs2_start(device, mode.stream, frame_callback, &data, require_no_error());
//    }
//
//    rs2_start_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 1 );
//
//    {
//        std::unique_lock<std::mutex> lk(m);
//        cv.wait(lk, [&]{return done;});
//        lk.unlock();
//    }
//
//    rs2_stop_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 0 );
//
//    for(auto & mode : modes)
//    {
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(data.duration_per_stream[mode.stream].end_time - data.duration_per_stream[mode.stream].start_time).count();
//        auto fps = ((float)data.number_of_frames_per_stream[mode.stream] / duration) * 1000.;
//        //check_fps(fps, mode.framerate); / TODO is not suppuorted yet. See above message
//    }
//}

//inline void motion_callback(rs2_device * , rs2_motion_data, void *)
//{
//}
//
//inline void timestamp_callback(rs2_device * , rs2_timestamp_data, void *)
//{
//}
//
//// Provide support for doing basic streaming tests on a set of specified modes
//inline void test_streaming(rs2_device * device, std::initializer_list<stream_profile> modes)
//{
//    std::map<rs2_stream, test_duration> duration_per_stream;
//    for(auto & mode : modes)
//    {
//        duration_per_stream.insert(std::pair<rs2_stream, test_duration>(mode.stream, test_duration()));
//        rs2_enable_stream(device, mode.stream, mode.width, mode.height, mode.format, mode.framerate, require_no_error());
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//    }
//
//    test_wait_for_frames(device, modes, duration_per_stream);
//
//    test_frame_callback(device, modes, duration_per_stream);
//
//    for(auto & mode : modes)
//    {
//        rs2_disable_stream(device, mode.stream, require_no_error());
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 0 );
//    }
//}

void test_option( rs2::sensor & device,
                  rs2_option option,
                  std::initializer_list< int > good_values,
                  std::initializer_list< int > bad_values )
{
    // Test reading the current value
    float first_value;
    REQUIRE_NOTHROW( first_value = device.get_option( option ) );


    // check if first value is something sane (should be default?)
    rs2::option_range range;
    REQUIRE_NOTHROW( range = device.get_option_range( option ) );
    REQUIRE( first_value >= range.min );
    REQUIRE( first_value <= range.max );
    CHECK( first_value == range.def );

    // Test setting good values, and that each value set can be subsequently get
    for( auto value : good_values )
    {
        REQUIRE_NOTHROW( device.set_option( option, (float)value ) );
        REQUIRE( device.get_option( option ) == value );
    }

    // Test setting bad values, and verify that they do not change the value of the option
    float last_good_value;
    REQUIRE_NOTHROW( last_good_value = device.get_option( option ) );
    for( auto value : bad_values )
    {
        REQUIRE_THROWS_AS( device.set_option( option, (float)value ), rs2::error );
        REQUIRE( device.get_option( option ) == last_good_value );
    }

    // Test that we can reset the option to its original value
    REQUIRE_NOTHROW( device.set_option( option, first_value ) );
    REQUIRE( device.get_option( option ) == first_value );
}


rs2::stream_profile get_profile_by_resolution_type( rs2::sensor & s, res_type res )
{
    auto sp = s.get_stream_profiles();
    int width = 0, height = 0;
    for( auto && p : sp )
    {
        if( auto video = p.as< rs2::video_stream_profile >() )
        {
            width = video.width();
            height = video.height();
            if( ( res = get_res_type( width, height ) ) )
                return p;
        }
    }
    std::stringstream ss;
    ss << "stream profile for " << width << "," << height << " resolution is not supported!";
    throw std::runtime_error( ss.str() );
}


std::shared_ptr< rs2::device > do_with_waiting_for_camera_connection( rs2::context ctx,
                                                                      std::shared_ptr< rs2::device > dev,
                                                                      std::string serial,
                                                                      std::function< void() > operation )
{
    std::mutex m;
    bool disconnected = false;
    bool connected = false;
    std::shared_ptr< rs2::device > result;
    std::condition_variable cv;

    ctx.set_devices_changed_callback( [&]( rs2::event_information info ) mutable {
        if( info.was_removed( *dev ) )
        {
            std::unique_lock< std::mutex > lock( m );
            disconnected = true;
            cv.notify_all();
        }
        auto list = info.get_new_devices();
        if( list.size() > 0 )
        {
            for( auto cam : list )
            {
                if( serial == cam.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) )
                {
                    std::unique_lock< std::mutex > lock( m );
                    connected = true;
                    result = std::make_shared< rs2::device >( cam );

                    disable_sensitive_options_for( *result );
                    cv.notify_all();
                    break;
                }
            }
        }
    } );

    operation();

    std::unique_lock< std::mutex > lock( m );
    REQUIRE( wait_for_reset(
        [&]() { return cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() { return disconnected; } ); },
        dev ) );
    REQUIRE( cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() { return connected; } ) );
    REQUIRE( result );
    ctx.set_devices_changed_callback( []( rs2::event_information info ) {} );  // reset callback

    return result;
}


rs2::depth_sensor restart_first_device_and_return_depth_sensor( const rs2::context & ctx,
                                                                const rs2::device_list & devices_list )
{
    rs2::device dev = devices_list[0];
    std::string serial;
    REQUIRE_NOTHROW( serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) );
    // forcing hardware reset to simulate device disconnection
    auto shared_dev = std::make_shared< rs2::device >( devices_list.front() );
    shared_dev = do_with_waiting_for_camera_connection( ctx, shared_dev, serial, [&]() { shared_dev->hardware_reset(); } );
    REQUIRE( shared_dev );
    rs2::depth_sensor depth_sensor = shared_dev->query_sensors().front();
    return depth_sensor;
}


#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <KnownFolders.h>
#include <shlobj.h>

std::string get_folder_path( special_folder f )
{
    std::string res;
    if( f == temp_folder )
    {
        TCHAR buf[MAX_PATH];
        if( GetTempPath( MAX_PATH, buf ) != 0 )
        {
            char str[1024];
            wcstombs( str, buf, 1023 );
            res = str;
        }
    }
    else
    {
        GUID folder;
        switch( f )
        {
        case user_desktop:
            folder = FOLDERID_Desktop;
            break;
        case user_documents:
            folder = FOLDERID_Documents;
            break;
        case user_pictures:
            folder = FOLDERID_Pictures;
            break;
        case user_videos:
            folder = FOLDERID_Videos;
            break;
        default:
            throw std::invalid_argument( std::string( "Value of f (" ) + std::to_string( f )
                                         + std::string( ") is not supported" ) );
        }
        PWSTR folder_path = NULL;
        HRESULT hr = SHGetKnownFolderPath( folder, KF_FLAG_DEFAULT_PATH, NULL, &folder_path );
        if( SUCCEEDED( hr ) )
        {
            char str[1024];
            wcstombs( str, folder_path, 1023 );
            CoTaskMemFree( folder_path );
            res = str;
        }
    }
    return res;
}
#endif //_WIN32

#if defined __linux__ || defined __APPLE__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

std::string get_folder_path( special_folder f )
{
    std::string res;
    if( f == special_folder::temp_folder )
    {
        const char * tmp_dir = getenv( "TMPDIR" );
        res = tmp_dir ? tmp_dir : "/tmp/";
    }
    else
    {
        const char * home_dir = getenv( "HOME" );
        if( ! home_dir )
        {
            struct passwd * pw = getpwuid( getuid() );
            home_dir = ( pw && pw->pw_dir ) ? pw->pw_dir : "";
        }
        if( home_dir )
        {
            res = home_dir;
            switch( f )
            {
            case user_desktop:
                res += "/Desktop/";
                break;
            case user_documents:
                res += "/Documents/";
                break;
            case user_pictures:
                res += "/Pictures/";
                break;
            case user_videos:
                res += "/Videos/";
                break;
            default:
                throw std::invalid_argument( std::string( "Value of f (" ) + std::to_string( f )
                                             + std::string( ") is not supported" ) );
            }
        }
    }
    return res;
}
#endif // defined __linux__ || defined __APPLE__
