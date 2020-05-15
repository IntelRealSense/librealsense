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

    // Copy of RunContext, but modified to run a custom lambda (see test_case()) and made a bit more
    // minimal:
    //      - no section support
    class CustomRunContext
        : public IResultCapture
        , public IRunner
    {
        CustomRunContext( CustomRunContext const& );
        void operator=( CustomRunContext const& );

    public:
        // More custom way to control the reporting is by specifying a custom reporter and configuration
        explicit CustomRunContext(
            Ptr< IStreamingReporter > const & reporter,
            Ptr< IConfig > const & cfg
        )
            : m_runInfo( {} )
            , m_config( cfg.get() )
            , m_reporter( reporter )
            , m_context( getCurrentMutableContext() )
            , m_test_info( {}, {}, {}, {}, {} )
        {
            m_context.setRunner( this );
            m_context.setConfig( m_config );
            m_context.setResultCapture( this );
            m_reporter->testRunStarting( m_runInfo );
        }

        // Easy way to instantiate, using the compact reporter by default
        explicit CustomRunContext( std::string const & reporter_type = "compact", Ptr< IConfig > const & cfg = new Config )
            : CustomRunContext( getRegistryHub().getReporterRegistry().create( reporter_type, cfg ), cfg )
        {
        }

        // Easy way to instantiate, using config data first
        explicit CustomRunContext( Ptr< IConfig > const & cfg, std::string const & reporter_type = "compact" )
            : CustomRunContext( reporter_type, cfg )
        {
        }

        virtual ~CustomRunContext()
        {
            m_reporter->testRunEnded( TestRunStats( m_runInfo, m_totals, aborting() ) );
            m_context.setRunner( nullptr );
            m_context.setConfig( nullptr );
            m_context.setResultCapture( nullptr );
        }

        void testGroupStarting( std::string const& testSpec, std::size_t groupIndex, std::size_t groupsCount )
        {
            m_reporter->testGroupStarting( GroupInfo( testSpec, groupIndex, groupsCount ) );
        }

        void testGroupEnded( std::string const& testSpec, Totals const& totals, std::size_t groupIndex, std::size_t groupsCount )
        {
            m_reporter->testGroupEnded( TestGroupStats( GroupInfo( testSpec, groupIndex, groupsCount ), totals, aborting() ) );
        }

        template< class T >
        Totals test_case( std::string const & name, T test )
        {
            Totals prev_totals = m_totals;
            m_test_info = TestCaseInfo( name, {}, {}, {}, {} );

            m_reporter->testCaseStarting( m_test_info );
            m_testCaseTracker = TestCaseTracker( m_test_info.name );

            std::string redirected_cout, redirected_cerr;


            try
            {
                m_lastAssertionInfo = AssertionInfo( "TEST_CASE", m_test_info.lineInfo, {}, ResultDisposition::Normal );
                TestCaseTracker::Guard guard( *m_testCaseTracker );

                if( m_reporter->getPreferences().shouldRedirectStdOut )
                {
                    StreamRedirect coutRedir( cout(), redirected_cout );
                    StreamRedirect cerrRedir( cerr(), redirected_cerr );
                    test();
                }
                else
                {
                    test();
                }
            }
            catch( TestFailureException & )
            {
                // This just means the test was aborted due to failure
            }
            catch( ... )
            {
                makeUnexpectedResultBuilder().useActiveException();
            }
            m_messages.clear();


            Totals delta = m_totals.delta( prev_totals );
            m_totals.testCases += delta.testCases;
            m_reporter->testCaseEnded(
                TestCaseStats( m_test_info,
                    delta,
                    redirected_cout,
                    redirected_cerr,
                    aborting()
                ) );

            m_testCaseTracker.reset();
            return delta;
        }

    private: // IResultCapture
        virtual void assertionEnded( AssertionResult const & result )
        {
            if( result.getResultType() == ResultWas::Ok )
                m_totals.assertions.passed++;
            else if( !result.isOk() )
                m_totals.assertions.failed++;

            if( m_reporter->assertionEnded( AssertionStats( result, m_messages, m_totals ) ) )
                m_messages.clear();

            // Reset working state
            m_lastAssertionInfo = AssertionInfo( {}, m_lastAssertionInfo.lineInfo, "{Unknown expression after the reported line}", m_lastAssertionInfo.resultDisposition );
            m_lastResult = result;
        }

        virtual bool sectionStarted( SectionInfo const & sectionInfo, Counts & assertions )
        {
            assert( false );
            return true;
        }

        virtual void sectionEnded( SectionInfo const & info, Counts const& prevAssertions, double _durationInSeconds )
        {
            assert( false );
        }

        virtual void pushScopedMessage( MessageInfo const& message )
        {
            m_messages.push_back( message );
        }

        virtual void popScopedMessage( MessageInfo const& message )
        {
            m_messages.erase( std::remove( m_messages.begin(), m_messages.end(), message ), m_messages.end() );
        }

        virtual std::string getCurrentTestName() const { return {}; }

        virtual const AssertionResult * getLastResult() const { return &m_lastResult; }

        virtual void handleFatalErrorCondition( std::string const & message )
        {
            ResultBuilder resultBuilder = makeUnexpectedResultBuilder();
            resultBuilder.setResultType( ResultWas::FatalErrorCondition );
            resultBuilder << message;
            resultBuilder.captureExpression();
            SectionInfo testCaseSection( m_test_info.lineInfo, m_test_info.name, m_test_info.description );

            Totals deltaTotals;
            deltaTotals.testCases.failed = 1;
            m_reporter->testCaseEnded( TestCaseStats( m_test_info,
                deltaTotals,
                {},
                {},
                false ) );
            m_totals.testCases.failed++;
            testGroupEnded( {}, m_totals, 1, 1 );
            m_reporter->testRunEnded( TestRunStats( m_runInfo, m_totals, false ) );
        }

    public:
        bool aborting() const { return m_totals.assertions.failed == static_cast<std::size_t>(m_config->abortAfter()); }

    private:
        ResultBuilder makeUnexpectedResultBuilder() const
        {
            return ResultBuilder( m_lastAssertionInfo.macroName.c_str(),
                m_lastAssertionInfo.lineInfo,
                m_lastAssertionInfo.capturedExpression.c_str(),
                m_lastAssertionInfo.resultDisposition );
        }

        Ptr< IConfig const > m_config;
        Totals m_totals;
        Ptr< IStreamingReporter > m_reporter;
        TestRunInfo m_runInfo;
        IMutableContext& m_context;

        TestCaseInfo m_test_info;
        Option< TestCaseTracker > m_testCaseTracker;
        AssertionResult m_lastResult;
        std::vector< MessageInfo > m_messages;
        AssertionInfo m_lastAssertionInfo;
    };

}  // namespace Catch

#endif // CATCH_CONFIG_RUNNER
