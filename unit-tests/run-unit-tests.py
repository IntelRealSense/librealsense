#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, locale, re, platform, getopt, time

# Remove Python's default list of places to look for modules!
# We want only modules in the directories we specifically provide to be found,
# otherwise pyrs other than what we compiled might be found...
sys.path = list()
sys.path.append( '' )  # directs Python to search modules in the current directory first
sys.path.append( os.path.dirname( sys.executable ) )
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'DLLs' ) )
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'lib' ) )
# Add our py/ module directory
current_dir = os.path.dirname( os.path.abspath( __file__ ) )
sys.path.append( current_dir + os.sep + "py" )

from rspy import log, file, repo, libci


def usage():
    ourname = os.path.basename( sys.argv[0] )
    print( 'Syntax: ' + ourname + ' [options] [dir]' )
    print( '        dir: the directory holding the executable tests to run (default to the build directory)' )
    print( 'Options:' )
    print( '        --debug          Turn on debugging information' )
    print( '        -v, --verbose    Errors will dump the log to stdout' )
    print( '        -q, --quiet      Suppress output; rely on exit status (0=no failures)' )
    print( '        -r, --regex      Run all tests that fit the following regular expression' )
    print( '        -s, --stdout     Do not redirect stdout to logs' )
    print( '        -t, --tag        Run all tests with the following tag. If used multiple times runs all tests matching' )
    print( '                         all tags. e.g. -t tag1 -t tag2 will run tests who have both tag1 and tag2' )
    print( '                         tests automatically get tagged with \'exe\' or \'py\' and based on their location' )
    print( '                         inside unit-tests/, e.g. unit-tests/func/test-hdr.py gets [func, py]' )
    print( '        --list-tags      Print out all available tags. This option will not run any tests' )
    print( '        --list-tests     Print out all available tests. This option will not run any tests' )
    print( '                         if both list-tags and list-tests are specified each test will be printed along' )
    print( '                         with what tags it has' )
    print( '        --no-exceptions  Do not load the LibCI/exceptions.specs file' )
    print( '        --context <>     The context to use for test configuration' )
    print( '        --repeat <#>     Repeat each test <#> times' )
    print( '        --config <>      Ignore test configurations; use the one provided' )
    print( '        --no-reset       Do not try to reset any devices, with or without Acroname' )
    print( '        --rslog          Enable LibRS logging (LOG_DEBUG etc.) to console in each test' )
    print()
    print( 'Examples:' )
    print( 'Running: python run-unit-tests.py -s' )
    print( '    Runs all tests, but direct their output to the console rather than log files' )
    print( 'Running: python run-unit-tests.py --list-tests --list-tags' )
    print( "    Will find all tests and print for each one what tags it has in the following format:" )
    print( '        <test-name> has tags: <tags-separated-by-spaces>' )
    print( 'Running: python run-unit-tests.py -r name -t log ~/my-build-directory' )
    print( "    Will run all tests whose name contains 'name' and who have the tag 'log' while searching for the" )
    print( "    exe files in the provided directory. Each test will create its own .log file to which its" )
    print( "    output will be written." )
    sys.exit( 2 )


# get os and directories for future use
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux' and "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

# Parse command-line:
try:
    opts, args = getopt.getopt( sys.argv[1:], 'hvqr:st:',
                                longopts=['help', 'verbose', 'debug', 'quiet', 'regex=', 'stdout', 'tag=', 'list-tags',
                                          'list-tests', 'no-exceptions', 'context=', 'repeat=', 'config=', 'no-reset',
                                          'rslog'] )
except getopt.GetoptError as err:
    log.e( err )  # something like "option -a not recognized"
    usage()
regex = None
to_stdout = False
required_tags = []
list_tags = False
list_tests = False
no_exceptions = False
context = None
repeat = 1
forced_configurations = None
no_reset = False
rslog = False
for opt, arg in opts:
    if opt in ('-h', '--help'):
        usage()
    elif opt in ('-v', '--verbose'):
        log.verbose_on()
    elif opt in ('-q', '--quiet'):
        log.quiet_on()
    elif opt in ('-r', '--regex'):
        regex = arg
    elif opt in ('-s', '--stdout'):
        to_stdout = True
    elif opt in ('-t', '--tag'):
        required_tags.append( arg )
    elif opt == '--list-tags':
        list_tags = True
    elif opt == '--list-tests':
        list_tests = True
    elif opt == '--no-exceptions':
        no_exceptions = True
    elif opt == '--context':
        context = arg
    elif opt == '--repeat':
        if not arg.isnumeric()  or  int(arg) < 1:
            log.e( "--repeat must be a number greater than 0" )
            usage()
        repeat = int(arg)
    elif opt == '--config':
        forced_configurations = [[arg]]
    elif opt == '--no-reset':
        no_reset = True
    elif opt == '--rslog':
        rslog = True

def find_build_dir( dir ):
    """
    Given a file we know must be within the build directory, go up in the tree until we find
    a file we know must be in the build directory itself...
    """
    build_dir = dir
    while True:
        if os.path.isfile( os.path.join( build_dir, 'CMakeCache.txt' )):
            log.d( 'assuming build dir path:', build_dir )
            return build_dir
        base = os.path.dirname( build_dir )
        if base == build_dir:
            log.d( 'could not find CMakeCache.txt; cannot assume build dir from', dir )
            break
        build_dir = base

if len( args ) > 1:
    usage()
target = None                 # the directory in which we expect to find exes
if len( args ) == 1:
    target = args[0]
    if not os.path.isdir( target ):
        log.f( 'Not a directory:', target )
    build_dir = find_build_dir( target )
else:
    build_dir = repo.build    # may not actually contain exes
    log.d( 'repo.build:', build_dir )

# Python scripts should be able to find the pyrealsense2 .pyd or else they won't work. We don't know
# if the user (Travis included) has pyrealsense2 installed but even if so, we want to use the one we compiled.
# we search the librealsense repository for the .pyd file (.so file in linux)
pyrs = ""
if linux:
    for so in file.find( target or build_dir or repo.root, '(^|/)pyrealsense2.*\.so$' ):
        pyrs = so
else:
    for pyd in file.find( target or build_dir or repo.root, '(^|/)pyrealsense2.*\.pyd$' ):
        pyrs = pyd

if pyrs:
    # After use of find, pyrs contains the path from the repo root to the pyrealsense that was found
    # We append it to the root path to get an absolute path and add to PYTHONPATH so it can be found by the tests
    pyrs_path = os.path.join( target or build_dir or repo.root, pyrs )
    # We need to add the directory not the file itself
    pyrs_path = os.path.dirname( pyrs_path )
    log.d( 'found pyrealsense pyd in:', pyrs_path )
    if not target:
        build_dir = find_build_dir( pyrs_path )
        if linux:
            target = build_dir
        else:
            target = pyrs_path

# Try to assume target directory from inside build directory. Only works if there is only one location with tests
if not target and build_dir:
    mask = r'(^|/)test-[^/.]*'
    if linux:
        mask += r'$'
    else:
        mask += r'\.exe'
    for executable in file.find( build_dir, mask ):
        executable = os.path.join( build_dir, executable )
        #log.d( 'found exe=', executable )
        if not file.is_executable( executable ):
            continue
        dir_with_test = os.path.dirname( executable )
        if target and target != dir_with_test:
            log.f( "Ambiguous executable tests in 2 directories:\n\t", target, "\n\t", dir_with_test,
                    "\n\tSpecify the directory manually..." )
        target = dir_with_test

if not to_stdout:
    if target:
        logdir = target + os.sep + 'unit-tests'
    else:  # no test executables were found. We put the logs directly in build directory
        logdir = os.path.join( repo.root, 'build', 'unit-tests' )
    os.makedirs( logdir, exist_ok=True )
    libci.logdir = logdir
n_tests = 0

# Figure out which sys.path we want the tests to see, assuming we have Python tests
#     PYTHONPATH is what Python will ADD to sys.path for the child processes
# (We can simply change `sys.path` but any child python scripts won't see it; we change the environment instead)
#
# We also need to add the path to the python packages that the tests use
os.environ["PYTHONPATH"] = current_dir + os.sep + "py"
#
if pyrs:
    os.environ["PYTHONPATH"] += os.pathsep + pyrs_path


def configuration_str( configuration, repetition = 1, prefix = '', suffix = '' ):
    """ Return a string repr (with a prefix and/or suffix) of the configuration or '' if it's None """
    s = ''
    if configuration is not None:
        s += '[' + ' '.join( configuration ) + ']'
    if repetition:
        s += '[' + str(repetition+1) + ']'
    if s:
        s = prefix + s + suffix
    return s


def check_log_for_fails( path_to_log, testname, configuration = None, repetition = 1 ):
    # Normal logs are expected to have in last line:
    #     "All tests passed (11 assertions in 1 test case)"
    # Tests that have failures, however, will show:
    #     "test cases: 1 | 1 failed
    #      assertions: 9 | 6 passed | 3 failed"
    # We make sure we look at the log written by the last run of the test by ignoring anything before the last
    # line with "----...---" that separate between 2 separate runs of he test
    if path_to_log is None:
        return False
    results = None
    for ctx in file.grep( r'^test cases:\s*(\d+) \|\s*(\d+) (passed|failed)|^-+$', path_to_log ):
        m = ctx['match']
        if m.string == "---------------------------------------------------------------------------------":
            results = None
        else:
            results = m

    if not results:
        return False

    total = int( results.group( 1 ) )
    passed = int( results.group( 2 ) )
    if results.group( 3 ) == 'failed':
        # "test cases: 1 | 1 failed"
        passed = total - passed
    if passed < total:
        if total == 1 or passed == 0:
            desc = 'failed'
        else:
            desc = str( total - passed ) + ' of ' + str( total ) + ' failed'

        if log.is_verbose_on():
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration, repetition, suffix=' ' ) + desc )
            log.i( 'Log: >>>' )
            log.out()
            file.cat( path_to_log )
            log.out( '<<<' )
        else:
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration, repetition,
                                                                              suffix=' ' ) + desc + '; see ' + path_to_log )
        return True
    return False


def get_tests():
    global regex, target, pyrs, current_dir, linux, context, list_only
    if regex:
        pattern = re.compile( regex )
    if list_only:
        # We want to list all tests, even if they weren't built.
        # So we look for the source files instead of using the manifest
        for cpp_test in file.find( current_dir, '(^|/)test-.*\.cpp' ):
            testparent = os.path.dirname( cpp_test )  # "log/internal" <-  "log/internal/test-all.py"
            if testparent:
                testname = 'test-' + testparent.replace( '/', '-' ) + '-' + os.path.basename( cpp_test )[
                                                                            5:-4]  # remove .cpp
            else:
                testname = os.path.basename( cpp_test )[:-4]

            if regex and not pattern.search( testname ):
                continue

            yield libci.ExeTest( testname )
    elif target:
        # In Linux, the build targets are located elsewhere than on Windows
        # Go over all the tests from a "manifest" we take from the result of the last CMake
        # run (rather than, for example, looking for test-* in the build-directory):
        manifestfile = os.path.join( build_dir, 'CMakeFiles', 'TargetDirectories.txt' )
        if linux:
            manifestfile = target + '/CMakeFiles/TargetDirectories.txt'
        # log.d( manifestfile )
        for manifest_ctx in file.grep( r'(?<=unit-tests/build/)\S+(?=/CMakeFiles/test-\S+.dir$)', manifestfile ):
            # We need to first create the test name so we can see if it fits the regex
            testdir = manifest_ctx['match'].group( 0 )  # "log/internal/test-all"
            # log.d( testdir )
            testparent = os.path.dirname( testdir )  # "log/internal"
            if testparent:
                testname = 'test-' + testparent.replace( '/', '-' ) + '-' + os.path.basename( testdir )[
                                                                            5:]  # "test-log-internal-all"
            else:
                testname = testdir  # no parent folder so we get "test-all"

            if regex and not pattern.search( testname ):
                continue

            if linux:
                exe = target + '/unit-tests/build/' + testdir + '/' + testname
            else:
                exe = target + '/' + testname + '.exe'

            yield libci.ExeTest( testname, exe, context )

    # Python unit-test scripts are in the same directory as us... we want to consider running them
    # (we may not if they're live and we have no pyrealsense2.pyd):
    for py_test in file.find( current_dir, '(^|/)test-.*\.py' ):
        testparent = os.path.dirname( py_test )  # "log/internal" <-  "log/internal/test-all.py"
        if testparent:
            testname = 'test-' + testparent.replace( '/', '-' ) + '-' + os.path.basename( py_test )[5:-3]  # remove .py
        else:
            testname = os.path.basename( py_test )[:-3]

        if regex and not pattern.search( testname ):
            continue

        yield libci.PyTest( testname, py_test, context )


def prioritize_tests( tests ):
    return sorted( tests, key=lambda t: t.config.priority )


def devices_by_test_config( test, exceptions ):
    """
    Yield <configuration,serial-numbers> pairs for each valid configuration under which the
    test should run.

    The <configuration> is a list of ('test:device') designations, e.g. ['L500*', 'D415'].
    The <serial-numbers> is a set of device serial-numbers that fit this configuration.

    :param test: The test (of class type Test) we're interested in
    """
    global forced_configurations
    for configuration in ( forced_configurations  or  test.config.configurations ):
        try:
            for serial_numbers in devices.by_configuration( configuration, exceptions ):
                yield configuration, serial_numbers
        except RuntimeError as e:
            if devices.acroname:
                log.e( log.red + test.name + log.reset + ': ' + str( e ) )
            else:
                log.w( log.yellow + test.name + log.reset + ': ' + str( e ) )
            continue


def test_wrapper( test, configuration = None, repetition = 1 ):
    global n_tests, rslog
    n_tests += 1
    #
    if not log.is_debug_on() or log.is_color_on():
        log.progress( configuration_str( configuration, repetition, suffix=' ' ) + test.name, '...' )
    #
    log_path = test.get_log()
    #
    opts = set()
    if rslog:
        opts.add( '--rslog' )
    try:
        test.run_test( configuration = configuration, log_path = log_path, opts = opts )
    except FileNotFoundError as e:
        log.e( log.red + test.name + log.reset + ':', str( e ) + configuration_str( configuration, repetition, prefix=' ' ) )
    except subprocess.TimeoutExpired:
        log.e( log.red + test.name + log.reset + ':', configuration_str( configuration, repetition, suffix=' ' ) + 'timed out' )
    except subprocess.CalledProcessError as cpe:
        if not check_log_for_fails( log_path, test.name, configuration, repetition ):
            # An unexpected error occurred
            log.e( log.red + test.name + log.reset + ':',
                   configuration_str( configuration, repetition, suffix=' ' ) + 'exited with non-zero value (' + str(
                       cpe.returncode ) + ')' )


# Run all tests
list_only = list_tags or list_tests
if not list_only:
    if pyrs:
        sys.path.append( pyrs_path )
    from rspy import devices

    devices.query()
    devices.map_unknown_ports()
    #
    # Under Travis, we'll have no devices and no acroname
    skip_live_tests = len( devices.all() ) == 0 and not devices.acroname
    #
    if not skip_live_tests:
        if not to_stdout:
            log.i( 'Logs in:', libci.logdir )
        exceptions = None
        if not no_exceptions and os.path.isfile( libci.exceptionsfile ):
            try:
                log.d( 'loading device exceptions from:', libci.exceptionsfile )
                log.debug_indent()
                exceptions = devices.load_specs_from_file( libci.exceptionsfile )
                exceptions = devices.expand_specs( exceptions )
                log.d( '==>', exceptions )
            finally:
                log.debug_unindent()
#
log.reset_errors()
available_tags = set()
tests = []
if context:
    log.d( 'running under context:', context )
for test in prioritize_tests( get_tests() ):
    #
    log.d( 'found', test.name, '...' )
    try:
        log.debug_indent()
        test.debug_dump()
        #
        if test.config.donotrun:
            continue
        #
        if required_tags and not all( tag in test.config.tags for tag in required_tags ):
            log.d( 'does not fit --tag:', test.config.tags )
            continue
        #
        available_tags.update( test.config.tags )
        tests.append( test )
        if list_only:
            n_tests += 1
            continue
        #
        if not test.is_live():
            for repetition in range(repeat):
                test_wrapper( test, repetition = repetition )
            continue
        #
        if skip_live_tests:
            log.w( test.name + ':', 'is live and there are no cameras; skipping' )
            continue
        #
        for configuration, serial_numbers in devices_by_test_config( test, exceptions ):
            for repetition in range(repeat):
                try:
                    log.d( 'configuration:', configuration )
                    log.debug_indent()
                    if not no_reset:
                        devices.enable_only( serial_numbers, recycle=True )
                except RuntimeError as e:
                    log.w( log.red + test.name + log.reset + ': ' + str( e ) )
                else:
                    test_wrapper( test, configuration, repetition )
                finally:
                    log.debug_unindent()
        #
    finally:
        log.debug_unindent()

log.progress()
#
if not n_tests:
    log.f( 'No unit-tests found!' )
#
if list_only:
    if list_tags and list_tests:
        for t in sorted( tests, key= lambda x: x.name ):
            print( t.name, "has tags:", ' '.join( t.config.tags ) )
    #
    elif list_tags:
        for t in sorted( list( available_tags ) ):
            print( t )
    #
    elif list_tests:
        for t in sorted( tests, key= lambda x: x.name ):
            print( t.name )
#
else:
    n_errors = log.n_errors()
    if n_errors:
        log.out( log.red + str( n_errors ) + log.reset, 'of', n_tests, 'test(s)',
                 log.red + 'failed!' + log.reset + log.clear_eos )
        sys.exit( 1 )
    #
    log.out( str( n_tests ) + ' unit-test(s) completed successfully' + log.clear_eos )
#
sys.exit( 0 )