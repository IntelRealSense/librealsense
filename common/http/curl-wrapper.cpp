// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#ifdef BUILD_WITH_LIBCURL
#include <curl/curl.h>
#include <mutex>
#endif

#include "curl-wrapper.h"
#include <rsutils/easylogging/easyloggingpp.h>


namespace rs2
{
    namespace http
    {

#ifndef BUILD_WITH_LIBCURL

        // Dummy - built without libcurl.
        curl_wrapper::curl_wrapper() : _curl( nullptr ) {}
        curl_wrapper::~curl_wrapper() {}
        bool curl_wrapper::get( const std::string &, const write_func &, const progress_func &, bool, long ) { return false; }
        bool curl_wrapper::post_json( const std::string &, const std::string &, const std::string &, long * out_http_status )
        {
            if( out_http_status ) *out_http_status = 0;
            return false;
        }
        bool curl_wrapper::post_stream( const std::string &, const std::string &, const write_func &, const std::string &,
                                         long, long * out_http_status, std::string * )
        {
            if( out_http_status ) *out_http_status = 0;
            return false;
        }

#else

        static const long CONNECT_TIMEOUT_SEC = 5;   // connect phase cap
        static const long UPLOAD_TIMEOUT_SEC = 15;   // overall upload cap so a stalled transfer can't hang
        static const curl_off_t PROGRESS_MIN_INTERVAL = 500000;  // 0.5s between progress callbacks (microseconds)

        // Forward a body chunk to the write_func; return bytes accepted (curl treats a short
        // count as a write error and aborts).
        static size_t write_callback( void * data, size_t size, size_t nmemb, void * user )
        {
            if( ! user )
                return 0;  // no sink -> signal a write error, aborting the transfer
            auto const & fn = *static_cast< curl_wrapper::write_func * >( user );
            size_t len = size * nmemb;
            return ( fn && fn( static_cast< const char * >( data ), len ) ) ? len : 0;
        }

        // Discard sink for uploads (we don't need the response body).
        static size_t discard_callback( void *, size_t size, size_t nmemb, void * ) { return size * nmemb; }

        // Shared setup for post_json()/post_stream(). Returns the header list (caller owns it,
        // curl_slist_free_all() after perform()), or nullptr if allocation failed. Deliberately
        // excludes FAILONERROR/FOLLOWLOCATION/the write sink - those differ per caller.
        static curl_slist * configure_post( CURL * curl, const std::string & url, const std::string & body,
                                             const std::string & extra_header, long overall_timeout_sec )
        {
            curl_slist * headers = curl_slist_append( nullptr, "Content-Type: application/json" );
            if( headers && ! extra_header.empty() )
            {
                curl_slist * with_extra = curl_slist_append( headers, extra_header.c_str() );
                if( ! with_extra )
                    curl_slist_free_all( headers );  // append failed without taking ownership
                headers = with_extra;
            }
            if( ! headers )
            {
                LOG_ERROR( "Failed to allocate curl headers" );
                return nullptr;
            }

            curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
            curl_easy_setopt( curl, CURLOPT_POST, 1L );
            curl_easy_setopt( curl, CURLOPT_POSTFIELDS, body.c_str() );
            curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (long)body.size() );
            curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
            curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SEC );
            curl_easy_setopt( curl, CURLOPT_TIMEOUT, overall_timeout_sec );
            curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1L );
            return headers;
        }

        struct progress_state { curl_wrapper::progress_func fn; CURL * curl; curl_off_t last_time; };

        // Throttled to one call per PROGRESS_MIN_INTERVAL; return non-zero to abort (curl convention).
        static int progress_callback( void * p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t )
        {
            if( ! p )
                return 0;  // no progress state -> keep the transfer going
            auto * st = static_cast< progress_state * >( p );
            curl_off_t curtime = 0;
            if( curl_easy_getinfo( st->curl, CURLINFO_TOTAL_TIME_T, &curtime ) == CURLE_OK )
                if( dltotal != 0 && curtime - st->last_time > PROGRESS_MIN_INTERVAL )
                {
                    st->last_time = curtime;
                    return ( st->fn && st->fn( static_cast< uint64_t >( dlnow ),
                                               static_cast< uint64_t >( dltotal ) ) ) ? 0 : 1;
                }
            return 0;
        }

        curl_wrapper::curl_wrapper()
        {
            // One-time libcurl init before the first curl_easy_init anywhere. Thread-safe on the
            // libcurl we build (>=7.84), so callers never touch curl global state themselves.
            static std::once_flag curl_global_once;
            std::call_once( curl_global_once, []() { curl_global_init( CURL_GLOBAL_DEFAULT ); } );
            _curl = curl_easy_init();
        }

        curl_wrapper::~curl_wrapper()
        {
            if( _curl )
                curl_easy_cleanup( static_cast< CURL * >( _curl ) );
        }

        bool curl_wrapper::get( const std::string & url, const write_func & on_data,
                                const progress_func & on_progress, bool insecure, long overall_timeout_sec )
        {
            if( ! _curl )
                return false;
            CURL * curl = static_cast< CURL * >( _curl );

            curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
            curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SEC );
            curl_easy_setopt( curl, CURLOPT_TIMEOUT, overall_timeout_sec );  // 0 = no cap (curl default)
            curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );  // follow HTTP 3xx redirects
            curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1L );
            curl_easy_setopt( curl, CURLOPT_FAILONERROR, 1L );     // fail on HTTP >= 400
            curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
            curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&on_data );
            if( insecure )
            {
                curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
                curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
            }

            progress_state st{ on_progress, curl, 0 };
            if( on_progress )
            {
                curl_easy_setopt( curl, CURLOPT_XFERINFOFUNCTION, progress_callback );
                curl_easy_setopt( curl, CURLOPT_XFERINFODATA, &st );
                curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 0L );
            }
            else
            {
                curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 1L );
            }

            auto res = curl_easy_perform( curl );
            if( res != CURLE_OK )
            {
                LOG_ERROR( "HTTP GET from " << url << " failed: " << curl_easy_strerror( res ) );
                return false;
            }
            return true;
        }

        bool curl_wrapper::post_json( const std::string & url, const std::string & body, const std::string & extra_header,
                                       long * out_http_status )
        {
            if( out_http_status ) *out_http_status = 0;
            if( ! _curl )
                return false;
            CURL * curl = static_cast< CURL * >( _curl );

            curl_slist * headers = configure_post( curl, url, body, extra_header, UPLOAD_TIMEOUT_SEC );
            if( ! headers )
                return false;

            curl_easy_setopt( curl, CURLOPT_FAILONERROR, 1L );
            curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, discard_callback );

            auto res = curl_easy_perform( curl );
            bool ok = ( res == CURLE_OK );
            if( ! ok )
                LOG_ERROR( "HTTP POST to " << url << " failed: " << curl_easy_strerror( res ) );

            if( out_http_status )
            {
                long status = 0;
                curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &status );
                *out_http_status = status;
            }

            curl_slist_free_all( headers );
            return ok;
        }

        bool curl_wrapper::post_stream( const std::string & url, const std::string & body, const write_func & on_data,
                                         const std::string & extra_header, long overall_timeout_sec,
                                         long * out_http_status, std::string * out_error_detail )
        {
            if( out_http_status ) *out_http_status = 0;
            if( out_error_detail ) out_error_detail->clear();
            if( ! _curl )
                return false;
            CURL * curl = static_cast< CURL * >( _curl );

            curl_slist * headers = configure_post( curl, url, body, extra_header, overall_timeout_sec );
            if( ! headers )
            {
                if( out_error_detail ) *out_error_detail = "Failed to allocate curl headers";
                return false;
            }

            curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
            // Deliberately no CURLOPT_FAILONERROR: unlike get()/post_json(), the caller here wants
            // the full response body (which may itself carry a server-sent error event) regardless
            // of HTTP status, and decides what to do based on out_http_status after the fact.
            curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
            curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&on_data );

            auto res = curl_easy_perform( curl );
            bool ok = ( res == CURLE_OK );
            if( ! ok )
            {
                LOG_ERROR( "HTTP POST (stream) to " << url << " failed: " << curl_easy_strerror( res ) );
                if( out_error_detail ) *out_error_detail = curl_easy_strerror( res );
            }

            if( out_http_status )
            {
                long status = 0;
                curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &status );
                *out_http_status = status;
            }

            curl_slist_free_all( headers );
            return ok;
        }

#endif  // BUILD_WITH_LIBCURL

    }
}
