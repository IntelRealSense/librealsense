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
    print( '        dir: the directory holding the executable tests to run (default to the build directory' )
    print( 'Options:' )
    print( '        --debug          Turn on debugging information' )
    print( '        -v, --verbose    Errors will dump the log to stdout' )
    print( '        -q, --quiet      Suppress output; rely on exit status (0=no failures)' )
    print( '        -r, --regex      run all tests that fit the following regular expression' )
    print( '        -s, --stdout     do not redirect stdout to logs' )
    print( '        -t, --tag        run all tests with the following tag. If used multiple times runs all tests matching' )
    print( '                         all tags. e.g. -t tag1 -t tag2 will run tests who have both tag1 and tag2' )
    print( '                         tests automatically get tagged with \'exe\' or \'py\' and based on their location' )
    print( '                         inside unit-tests/, e.g. unit-tests/func/test-hdr.py gets [func, py]' )
    print( '        --list-tags      print out all available tags. This option will not run any tests' )
    print( '        --list-tests     print out all available tests. This option will not run any tests' )
    print( '                         if both list-tags and list-tests are specified each test will be printed along' )
    print( '                         with what tags it has' )
    print( '        --no-exceptions  do not load the LibCI/exceptions.specs file' )
    print( '        --context        The context to use for test configuration' )
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
                                          'list-tests', 'no-exceptions', 'context='] )
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

if len( args ) > 1:
    usage()
target = None
if len( args ) == 1:
    if os.path.isdir( args[0] ):
        target = args[0]
    else:
        usage()
# Trying to assume target directory from inside build directory. Only works if there is only one location with tests
if not target:
    build = repo.root + os.sep + 'build'
    for executable in file.find( build, '(^|/)test-.*' ):
        if not file.is_executable( executable ):
            continue
        dir_with_test = build + os.sep + os.path.dirname( executable )
        if target and target != dir_with_test:
            log.e( "Found executable tests in 2 directories:", target, "and", dir_with_test,
                   ". Can't default to directory" )
            usage()
        target = dir_with_test

if not to_stdout:
    if target:
        logdir = target + os.sep + 'unit-tests'
    else:  # no test executables were found. We put the logs directly in build directory
        logdir = os.path.join( repo.root, 'build', 'unit-tests' )
    os.makedirs( logdir, exist_ok=True )
    libci.logdir = logdir
n_tests = 0

# Python scripts should be able to find the pyrealsense2 .pyd or else they won't work. We don't know
# if the user (Travis included) has pyrealsense2 installed but even if so, we want to use the one we compiled.
# we search the librealsense repository for the .pyd file (.so file in linux)
pyrs = ""
if linux:
    for so in file.find( repo.root, '(^|/)pyrealsense2.*\.so$' ):
        pyrs = so
else:
    for pyd in file.find( repo.root, '(^|/)pyrealsense2.*\.pyd$' ):
        pyrs = pyd

if pyrs:
    # After use of find, pyrs contains the path from librealsense to the pyrealsense that was found
    # We append it to the librealsense path to get an absolute path to the file to add to PYTHONPATH so it can be found by the tests
    pyrs_path = repo.root + os.sep + pyrs
    # We need to add the directory not the file itself
    pyrs_path = os.path.dirname( pyrs_path )
    log.d( 'found pyrealsense pyd in:', pyrs_path )
    if not target:
        target = pyrs_path
        log.d( 'assuming executable path same as pyd path' )

# Figure out which sys.path we want the tests to see, assuming we have Python tests
#     PYTHONPATH is what Python will ADD to sys.path for the child processes
# (We can simply change `sys.path` but any child python scripts won't see it; we change the environment instead)
#
# We also need to add the path to the python packages that the tests use
os.environ["PYTHONPATH"] = current_dir + os.sep + "py"
#
if pyrs:
    os.environ["PYTHONPATH"] += os.pathsep + pyrs_path


def configuration_str( configuration, prefix = '', suffix = '' ):
    """ Return a string repr (with a prefix and/or suffix) of the configuration or '' if it's None """
    if configuration is None:
        return ''
    return prefix + '[' + ' '.join( configuration ) + ']' + suffix


def check_log_for_fails( path_to_log, testname, configuration = None ):
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
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration, suffix=' ' ) + desc )
            log.i( 'Log: >>>' )
            log.out()
            file.cat( path_to_log )
            log.out( '<<<' )
        else:
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration,
                                                                              suffix=' ' ) + desc + '; see ' + path_to_log )
        return True
    return False


def get_tests():
    global regex, target, pyrs, current_dir, linux
    if regex:
        pattern = re.compile( regex )
    if target:
        # In Linux, the build targets are located elsewhere than on Windows
        # Go over all the tests from a "manifest" we take from the result of the last CMake
        # run (rather than, for example, looking for test-* in the build-directory):
        if linux:
            manifestfile = target + '/CMakeFiles/TargetDirectories.txt'
        else:
            manifestfile = target + '/../CMakeFiles/TargetDirectories.txt'
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
    for configuration in test.config.configurations:
        try:
            for serial_numbers in devices.by_configuration( configuration, exceptions ):
                yield configuration, serial_numbers
        except RuntimeError as e:
            if devices.acroname:
                log.e( log.red + test.name + log.reset + ': ' + str( e ) )
            else:
                log.w( log.yellow + test.name + log.reset + ': ' + str( e ) )
            continue


def test_wrapper( test, configuration = None ):
    global n_tests
    n_tests += 1
    #
    if not log.is_debug_on() or log.is_color_on():
        log.progress( configuration_str( configuration, suffix=' ' ) + test.name, '...' )
    #
    log_path = test.get_log()
    try:
        test.run_test( configuration=configuration, log_path=log_path )
    except FileNotFoundError as e:
        log.e( log.red + test.name + log.reset + ':', str( e ) + configuration_str( configuration, prefix=' ' ) )
    except subprocess.TimeoutExpired:
        log.e( log.red + test.name + log.reset + ':', configuration_str( configuration, suffix=' ' ) + 'timed out' )
    except subprocess.CalledProcessError as cpe:
        if not check_log_for_fails( log_path, test.name, configuration ):
            # An unexpected error occurred
            log.e( log.red + test.name + log.reset + ':',
                   configuration_str( configuration, suffix=' ' ) + 'exited with non-zero value (' + str(
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
            test_wrapper( test )
            continue
        #
        if skip_live_tests:
            log.w( test.name + ':', 'is live and there are no cameras; skipping' )
            continue
        #
        for configuration, serial_numbers in devices_by_test_config( test, exceptions ):
            try:
                log.d( 'configuration:', configuration )
                log.debug_indent()
                devices.enable_only( serial_numbers, recycle=True )
            except RuntimeError as e:
                log.w( log.red + test.name + log.reset + ': ' + str( e ) )
            else:
                test_wrapper( test, configuration )
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
