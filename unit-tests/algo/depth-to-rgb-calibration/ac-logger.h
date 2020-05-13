#pragma once

#include <librealsense2/h/rs_types.h>  // rs2_log_severity
#include <string>
#include <iostream>
#include <easylogging++.h>


#define AC_LOG_PREFIX "AC1: "
#define AC_LOG_PREFIX_LEN 5
#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) do { std::string msg = librealsense::to_string() << AC_LOG_PREFIX << MSG; rs2_log( RS2_LOG_SEVERITY_##TYPE, msg.c_str(), nullptr ); } while( false )
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) TRACE( "-" << #TYPE [0] << "- " << MSG )


// This logger will catch all the realsense logs and output them to the
// screen
class ac_logger
{
    struct dispatch : public el::LogDispatchCallback
    {
        bool * _p_on;
        
        void handle( const el::LogDispatchData* data ) noexcept override
        {
            if( !*_p_on )
                return;
            char const * raw = data->logMessage()->message().data();
            if( !strncmp( AC_LOG_PREFIX, raw, AC_LOG_PREFIX_LEN ) )
            {
                std::cout << "-" << *el::LevelHelper::convertToString( data->logMessage()->level() ) << "- ";
                std::cout << (raw + AC_LOG_PREFIX_LEN) << std::endl;
            }
        }
    };

    bool _on;

public:
    explicit ac_logger( bool on = true )
        : _on( on )
    {
        el::Loggers::getLogger( "librealsense" );

        el::Helpers::installLogDispatchCallback< dispatch >( "our_dispatch" );
        auto dispatcher = el::Helpers::logDispatchCallback< dispatch >( "our_dispatch" );
        dispatcher->_p_on = &_on;
        el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );
    }

    void enable( bool on = true ) { _on = on; }

};
