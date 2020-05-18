// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// Catch defines CHECK() and so does EL++, and so we have to undefine it or we get compilation errors!
#undef CHECK

// Let Catch define its own main() function
#if ! defined( NO_CATCH_CONFIG_MAIN )
#define CATCH_CONFIG_MAIN
#endif
#ifdef _MSC_VER
/*
The .hpp gives the following warning C4244:
C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.16.27023\include\algorithm(2583): warning C4244: 'argument': conversion from '_Diff' to 'int', possible loss of data
C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.16.27023\include\algorithm(2607): note: see reference to function template instantiation 'void std::_Random_shuffle1<_RanIt,Catch::TestRegistry::RandomNumberGenerator>(_RanIt,_RanIt,_RngFn &)' being compiled
        with
        [
            _RanIt=std::_Vector_iterator<std::_Vector_val<std::_Simple_types<Catch::TestCase>>>,
            _RngFn=Catch::TestRegistry::RandomNumberGenerator
        ]
c:\work\git\lrs\unit-tests\algo\../catch/catch.hpp(5788): note: see reference to function template instantiation 'void std::random_shuffle<std::_Vector_iterator<std::_Vector_val<std::_Simple_types<_Ty>>>,Catch::TestRegistry::RandomNumberGenerator&>(_RanIt,_RanIt,_RngFn)' being compiled
        with
        [
            _Ty=Catch::TestCase,
            _RanIt=std::_Vector_iterator<std::_Vector_val<std::_Simple_types<Catch::TestCase>>>,
            _RngFn=Catch::TestRegistry::RandomNumberGenerator &
        ]
*/
#pragma warning (push)
#pragma warning (disable : 4244)    // 'this': used in base member initializer list
#endif
#include "catch/catch.hpp"
#ifdef _MSC_VER
#pragma warning (pop)
#endif


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
