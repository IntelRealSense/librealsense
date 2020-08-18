#pragma once

#include <librealsense2/h/rs_types.h>  // rs2_log_severity
#include <string>
#include <iostream>
#include <easylogging++.h>
#include "../../../src/algo/depth-to-rgb-calibration/debug.h"


// This logger will catch all the realsense logs and output them to the
// screen
class ac_logger
{
    struct dispatch : public el::LogDispatchCallback
    {
        ac_logger * _p_logger;
        
        void handle( const el::LogDispatchData* data ) noexcept override
        {
            if( ! _p_logger->_on )
                return;
            char const * raw = data->logMessage()->message().data();
            if( ! strncmp( AC_LOG_PREFIX, raw, AC_LOG_PREFIX_LEN ) )
            {
                _p_logger->on_log( *el::LevelHelper::convertToString( data->logMessage()->level() ),
                                   raw + AC_LOG_PREFIX_LEN );
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
        dispatcher->_p_logger = this;
        el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );
    }
    virtual ~ac_logger() {}

    void enable( bool on = true ) { _on = on; }

protected:
    virtual void on_log( char severity, char const * message )
    {
        std::cout << "-" << severity << "- ";
        std::cout << message << std::endl;
    }
};
