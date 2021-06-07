// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// Catch defines CHECK() and so does EL++, and so we have to undefine it or we get compilation errors!
#undef CHECK
// Otherwise, don't let it define its own:
#define ELPP_NO_CHECK_MACROS

// We set our own main by default, so CATCH_CONFIG_MAIN will cause linker errors!
#if defined(CATCH_CONFIG_MAIN)
#error CATCH_CONFIG_MAIN must not be defined: to run your own main(), use CATCH_CONFIG_RUNNER with #cmake:custom-main
#endif

#include "catch/catch.hpp"

#if defined(CATCH_CONFIG_RUNNER)

namespace Catch
{

    // Allow custom test-cases on the fly
    class CustomRunContext : public RunContext
    {
    public:
        CustomRunContext( CustomRunContext const& ) = delete;
        CustomRunContext & operator=( CustomRunContext const& ) = delete;

        // RunContext ctor, but you need to give details...
        explicit CustomRunContext( IStreamingReporterPtr&& reporter, IConfigPtr const& cfg )
            : RunContext( cfg, std::move( reporter ))
        {
        }

        // Easy way to instantiate, using the compact reporter by default
        explicit CustomRunContext( std::string const & reporter_type = "compact", IConfigPtr const & cfg = IConfigPtr( new Config ) )
            : CustomRunContext( getRegistryHub().getReporterRegistry().create( reporter_type, cfg ), cfg )
        {
        }

        // Easy way to instantiate, using config data first
        explicit CustomRunContext( Catch::ConfigData const & cfg, std::string const & reporter_type = "compact" )
            : CustomRunContext( reporter_type, IConfigPtr( new Config( cfg ) ) )
        {
        }

        // Allow changing the redirection for the reporter by force (the compact reporter does
        // not allow changing via the Config's verbosity)
        void set_redirection( bool on )
        {
            auto r = dynamic_cast<Catch::CompactReporter *>(&reporter());
            if( r )
                r->m_reporterPrefs.shouldRedirectStdOut = on;
        }

        template< class T >
        Totals run_test( std::string const & name, T test )
        {
            struct invoker : ITestInvoker
            {
                T _test;
                invoker( T t ) : _test( t ) {}
                void invoke() const override
                {
                    _test();
                }
            };
            TestCase test_case( new invoker( test ),
                TestCaseInfo( name, {}, {}, {}, {"",0} )
            );
            return runTest( test_case );
        }
    };

}  // namespace Catch

#endif // CATCH_CONFIG_RUNNER
