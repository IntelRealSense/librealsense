#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, locale, re, platform, getopt, time
from abc import ABC, abstractmethod

# Remove Python's default list of places to look for modules!
# We want only modules in the directories we specifically provide to be found,
# otherwise pyrs other than what we compiled might be found...
sys.path = list()
sys.path.append( '' )  # directs Python to search modules in the current directory first
sys.path.append( os.path.dirname( sys.executable ))
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'DLLs' ))
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'lib' ))
# Add our py/ module directory
current_dir = os.path.dirname( os.path.abspath( __file__ ))
sys.path.append( current_dir + os.sep + "py" )

from rspy import log, file, repo

def usage():
    ourname = os.path.basename(sys.argv[0])
    print( 'Syntax: ' + ourname + ' [options] [dir]' )
    print( '        dir: the directory holding the executable tests to run (default to the build directory')
    print( 'Options:' )
    print( '        --debug        Turn on debugging information' )
    print( '        -v, --verbose  Errors will dump the log to stdout' )
    print( '        -q, --quiet    Suppress output; rely on exit status (0=no failures)' )
    print( '        -r, --regex    run all tests that fit the following regular expression' )
    print( '        -s, --stdout   do not redirect stdout to logs' )
    print( '        -t, --tag      run all tests with the following tag' )
    print( '        --list-tags    print out all available tags. This option will not run any tests')
    print( '        --list-tests   print out all available tests. This option will not run any tests')
    sys.exit(2)

# get os and directories for future use
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

# Parse command-line:
try:
    opts,args = getopt.getopt( sys.argv[1:], 'hvqr:st:',
        longopts = [ 'help', 'verbose', 'debug', 'quiet', 'regex=', 'stdout', 'tag=', 'list-tags', 'list-tests' ])
except getopt.GetoptError as err:
    log.e( err )   # something like "option -a not recognized"
    usage()
regex = None
to_stdout = False
tag = None
list_tags = False
list_tests = False
for opt,arg in opts:
    if opt in ('-h','--help'):
        usage()
    elif opt in ('-v','--verbose'):
        log.verbose_on()
    elif opt in ('-q','--quiet'):
        log.quiet_on()
    elif opt in ('-r', '--regex'):
        regex = arg
    elif opt in ('-s', '--stdout'):
        to_stdout = True
    elif opt in ('-t', '--tag'):
        tag = arg
    elif opt == '--list-tags':
        list_tags = True
    elif opt == '--list-tests':
        list_tests = True

if len(args) > 1:
    usage()
target = None
if len(args) == 1:
    if os.path.isdir( args[0] ):
        target = args[0]
    else:
        usage()
# Trying to assume target directory from inside build directory. Only works if there is only one location with tests
if not target:
    build = repo.root + os.sep + 'build'
    for executable in file.find(build, '(^|/)test-.*'):
        if not file.is_executable(executable):
            continue
        dir_with_test = build + os.sep + os.path.dirname(executable)
        if target and target != dir_with_test:
            log.e("Found executable tests in 2 directories:", target, "and", dir_with_test, ". Can't default to directory")
            usage()
        target = dir_with_test

if target:
    logdir = target + os.sep + 'unit-tests'
else: # no test executables were found. We put the logs directly in build directory
    logdir = os.path.join( repo.root, 'build', 'unit-tests' )
os.makedirs( logdir, exist_ok = True )
n_tests = 0

# Python scripts should be able to find the pyrealsense2 .pyd or else they won't work. We don't know
# if the user (Travis included) has pyrealsense2 installed but even if so, we want to use the one we compiled.
# we search the librealsense repository for the .pyd file (.so file in linux)
pyrs = ""
if linux:
    for so in file.find(repo.root, '(^|/)pyrealsense2.*\.so$'):
        pyrs = so
else:
    for pyd in file.find(repo.root, '(^|/)pyrealsense2.*\.pyd$'):
        pyrs = pyd

if pyrs:
    # After use of find, pyrs contains the path from librealsense to the pyrealsense that was found
    # We append it to the librealsense path to get an absolute path to the file to add to PYTHONPATH so it can be found by the tests
    pyrs_path = repo.root + os.sep + pyrs
    # We need to add the directory not the file itself
    pyrs_path = os.path.dirname(pyrs_path)
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

def subprocess_run(cmd, stdout = None, timeout = 200, append = False):
    """
    Wrapper function for subprocess.run.
    If the child process times out or ends with a non-zero exit status an exception is raised!
    
    :param cmd: the command and argument for the child process, as a list
    :param stdout: path of file to direct the output of the process to (None to disable)
    :param timeout: number of seconds to give the process before forcefully ending it (None to disable)
    :param append: if True and stdout is not None, the log of the test will be appended to the file instead of
                   overwriting it
    :return: the output written by the child, if stdout is None -- otherwise N/A
    """
    log.d( 'running:', cmd )
    handle = None
    start_time = time.time()
    try:
        log.debug_indent()
        if stdout  and  stdout != subprocess.PIPE:
            if append:
                handle = open(stdout, "a" )
                handle.write("\n---------------------------------------------------------------------------------\n\n")
                handle.flush()
            else:
                handle = open( stdout, "w" )
            stdout = handle
        rv = subprocess.run( cmd,
                             stdout = stdout,
                             stderr = subprocess.STDOUT,
                             universal_newlines = True,
                             timeout = timeout,
                             check = True )
        result = rv.stdout
        if not result:
            result = []
        else:
            result = result.split( '\n' )
        return result
    finally:
        if handle:
            handle.close()
        log.debug_unindent()
        run_time = time.time() - start_time
        log.d("test took", run_time, "seconds")


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

    total = int(results.group(1))
    passed = int(results.group(2))
    if results.group(3) == 'failed':
        # "test cases: 1 | 1 failed"
        passed = total - passed
    if passed < total:
        if total == 1  or  passed == 0:
            desc = 'failed'
        else:
            desc = str(total - passed) + ' of ' + str(total) + ' failed'

        if log.is_verbose_on():
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration, suffix = ' ' ) + desc )
            log.i( 'Log: >>>' )
            log.out()
            file.cat( path_to_log )
            log.out( '<<<' )
        else:
            log.e( log.red + testname + log.reset + ': ' + configuration_str( configuration, suffix = ' ' ) + desc + '; see ' + path_to_log )
        return True
    return False


class TestConfig(ABC):  # Abstract Base Class
    """
    Configuration for a test, encompassing any metadata needed to control its run, like retries etc.
    """
    def __init__(self):
        self._configurations = list()
        self._priority = 1000
        self._tags = set()
        self._flags = set()
        self._timeout = 200

    def debug_dump(self):
        if self._priority != 1000:
            log.d( 'priority:', self._priority )
        if self._timeout != 200:
            log.d( 'timeout:', self._timeout)
        if self._tags:
            log.d( 'tags:', self._tags )
        if self._flags:
            log.d( 'flags:', self._flags )
        if len(self._configurations) > 1:
            log.d( len( self._configurations ), 'configurations' )
            # don't show them... they are output separately

    @property
    def configurations( self ):
        return self._configurations

    @property
    def priority( self ):
        return self._priority

    @property
    def timeout( self ):
        return self._timeout

    @property
    def tags( self ):
        return self._tags

    @property
    def flags( self ):
        return self._flags


class TestConfigFromText(TestConfig):
    """
    Configuration for a test -- from any text-based syntax with a given prefix, e.g. for python:
        #test:usb2
        #test:device L500* D400*
        #test:retries 3
        #test:priority 0
    And, for C++ the prefix could be:
        //#test:...
    """
    def __init__( self, source, line_prefix ):
        """
        :param source: The path to the text file
        :param line_prefix: A regex to denote a directive (must be first thing in a line), which will
            be immediately followed by the directive itself and optional arguments
        """
        TestConfig.__init__(self)

        # Parse the python
        regex = r'^' + line_prefix + r'(\S+)((?:\s+\S+)*?)\s*(?:#\s*(.*))?$'
        for context in file.grep( regex, source ):
            match = context['match']
            directive = match.group(1)
            params = [s for s in context['match'].group(2).split()]
            comment = match.group(3)
            if directive == 'device':
                #log.d( '    configuration:', params )
                if not params:
                    log.e( source + '+' + str(context['index']) + ': device directive with no devices listed' )
                else:
                    self._configurations.append( params )
            elif directive == 'priority':
                if len(params) == 1 and params[0].isdigit():
                    self._priority = int( params[0] )
                else:
                    log.e( source + '+' + str(context['index']) + ': priority directive with invalid parameters:', params )
            elif directive == 'timeout':
                if len(params) == 1 and params[0].isdigit():
                    self._timeout = int( params[0] )
                else:
                    log.e( source + '+' + str(context['index']) + ': timeout directive with invalid parameters:', params )
            elif directive == 'tag':
                self._tags.update(params)
            elif directive == 'flag':
                self._flags.update( params )
            else:
                log.e( source + '+' + str(context['index']) + ': invalid directive "' + directive + '"; ignoring' )


class Test(ABC):  # Abstract Base Class
    """
    Abstract class for a test. Holds the name of the test
    """
    def __init__(self, testname):
        #log.d( 'found', testname )
        self._name = testname
        self._config = None
        self._ran = False

    @abstractmethod
    def run_test( self, configuration = None, log_path = None ):
        pass

    def debug_dump( self ):
        if self._config:
            self._config.debug_dump()

    @property
    def config( self ):
        return self._config

    @property
    def name( self ):
        return self._name

    @property
    def ran( self ):
        return self._ran

    def get_log( self ):
        global to_stdout
        if to_stdout:
            path = None
        else:
            path = logdir + os.sep + self.name + ".log"
        return path

    def is_live( self ):
        """
        Returns True if the test configurations specify devices (test has a 'device' directive)
        """
        return self._config and len(self._config.configurations) > 0


class PyTest(Test):
    """
    Class for python tests. Hold the path to the script of the test
    """
    def __init__(self, testname, path_to_test):
        """
        :param testname: name of the test
        :param path_to_test: the relative path from the current directory to the path
        """
        global current_dir
        Test.__init__(self, testname)
        self.path_to_script = current_dir + os.sep + path_to_test
        self._config = TestConfigFromText( self.path_to_script, r'#\s*test:' )

    def debug_dump(self):
        log.d( 'script:', self.path_to_script )
        Test.debug_dump(self)

    @property
    def command(self):
        cmd = [sys.executable]
        # The unit-tests should only find module we've specifically added -- but Python may have site packages
        # that are automatically made available. We want to avoid those:
        #     -S     : don't imply 'import site' on initialization
        # NOTE: exit() is defined in site.py and works only if the site module is imported!
        cmd += ['-S']
        if sys.flags.verbose:
            cmd += ["-v"]
        cmd += [self.path_to_script]
        if 'custom-args' not in self.config.flags:
            if log.is_debug_on():
                cmd += ['--debug']
            if log.is_color_on():
                cmd += ['--color']
        return cmd

    def run_test( self, configuration = None, log_path = None ):
        try:
            subprocess_run( self.command, stdout=log_path, append=self.ran, timeout=self.config.timeout )
        finally:
            self._ran = True


class ExeTest(Test):
    """
    Class for c/cpp tests. Hold the path to the executable for the test
    """
    def __init__( self, testname, exe ):
        """
        :param testname: name of the test
        :param exe: full path to executable
        """
        global current_dir
        Test.__init__(self, testname)
        self.exe = exe

        # Finding the c/cpp file of the test to get the configuration
        # TODO: this is limited to a structure in which .cpp files and directories do not share names
        # For example:
        #     unit-tests/
        #         func/
        #             ...
        #         test-func.cpp
        # test-func.cpp will not be found!
        split_testname = testname.split( '-' )
        cpp_path = current_dir
        found_test_dir = False

        while not found_test_dir:
            # index 0 should be 'test' as tests always start with it
            found_test_dir = True
            for i in range(2, len(split_testname) ): # Checking if the next part of the test name is a sub-directory
                sub_dir_path = cpp_path + os.sep + '-'.join(split_testname[1:i]) # The next sub-directory could have several words
                if os.path.isdir(sub_dir_path):
                    cpp_path = sub_dir_path
                    del split_testname[1:i]
                    found_test_dir = False
                    break

        cpp_path += os.sep + '-'.join( split_testname )
        if os.path.isfile( cpp_path + ".cpp" ):
            cpp_path += ".cpp"
            self._config = TestConfigFromText(cpp_path, r'//#\s*test:')
        else:
            log.w( log.red + testname + log.reset + ':', 'No matching .cpp file was found; no configuration will be used!' )

    @property
    def command(self):
        cmd = [self.exe]
        if 'custom-args' not in self.config.flags:
            # Assume we're a Catch2 exe, so:
            #if sys.flags.verbose:
            #    cmd += 
            if log.is_debug_on():
                cmd += ['-d', 'yes']  # show durations for each test-case
                #cmd += ['--success']  # show successful assertions in output
            #if log.is_color_on():
            #    cmd += ['--use-colour', 'yes']
        return cmd

    def run_test( self, configuration = None, log_path = None ):
        try:
            subprocess_run( self.command, stdout=log_path, append=self.ran, timeout=self.config.timeout )
        finally:
            self._ran = True


def get_tests():
    global regex, target, pyrs, current_dir, linux
    if regex:
        pattern = re.compile(regex)
    if target:
        # In Linux, the build targets are located elsewhere than on Windows
        # Go over all the tests from a "manifest" we take from the result of the last CMake
        # run (rather than, for example, looking for test-* in the build-directory):
        if linux:
            manifestfile = target + '/CMakeFiles/TargetDirectories.txt'
        else:
            manifestfile = target + '/../CMakeFiles/TargetDirectories.txt'
        #log.d( manifestfile )
        for manifest_ctx in file.grep(r'(?<=unit-tests/build/)\S+(?=/CMakeFiles/test-\S+.dir$)', manifestfile):
            # We need to first create the test name so we can see if it fits the regex
            testdir = manifest_ctx['match'].group(0)  # "log/internal/test-all"
            #log.d( testdir )
            testparent = os.path.dirname(testdir)  # "log/internal"
            if testparent:
                testname = 'test-' + testparent.replace('/', '-') + '-' + os.path.basename(testdir)[5:]  # "test-log-internal-all"
            else:
                testname = testdir  # no parent folder so we get "test-all"

            if regex and not pattern.search( testname ):
                continue

            if linux:
                exe = target + '/unit-tests/build/' + testdir + '/' + testname
            else:
                exe = target + '/' + testname + '.exe'

            yield ExeTest(testname, exe)

    # Python unit-test scripts are in the same directory as us... we want to consider running them
    # (we may not if they're live and we have no pyrealsense2.pyd):
    for py_test in file.find(current_dir, '(^|/)test-.*\.py'):
        testparent = os.path.dirname(py_test)  # "log/internal" <-  "log/internal/test-all.py"
        if testparent:
            testname = 'test-' + testparent.replace('/', '-') + '-' + os.path.basename(py_test)[5:-3]  # remove .py
        else:
            testname = os.path.basename(py_test)[:-3]

        if regex and not pattern.search( testname ):
            continue

        yield PyTest(testname, py_test)


def prioritize_tests( tests ):
    return sorted(tests, key= lambda t: t.config.priority)

def devices_by_test_config( test ):
    """
    Yield <configuration,serial-numbers> pairs for each valid configuration under which the
    test should run.

    The <configuration> is a list of ('test:device') designations, e.g. ['L500*', 'D415'].
    The <serial-numbers> is a set of device serial-numbers that fit this configuration.

    :param test: The test (of class type Test) we're interested in
    """
    for configuration in test.config.configurations:
        try:
            serial_numbers = devices.by_configuration( configuration )
        except RuntimeError as e:
            if devices.acroname:
                log.e( log.red + test.name + log.reset + ': ' + str(e) )
            else:
                log.w( log.yellow + test.name + log.reset + ': ' + str(e) )
            continue
        yield configuration, serial_numbers


log.i( 'Logs in:', logdir )
def test_wrapper( test, configuration = None ):
    global n_tests
    n_tests += 1
    #
    if not log.is_debug_on()  or  log.is_color_on():
        log.progress( configuration_str( configuration, suffix = ' ' ) + test.name, '...' )
    #
    log_path = test.get_log()
    try:
        test.run_test( configuration = configuration, log_path = log_path )
    except FileNotFoundError:
        log.e( log.red + test.name + log.reset + ':', str(e) + configuration_str( configuration, prefix = ' ' ) )
    except subprocess.TimeoutExpired:
        log.e(log.red + test.name + log.reset + ':', configuration_str(configuration, suffix=' ') + 'timed out')
    except subprocess.CalledProcessError as cpe:
        if not check_log_for_fails( log_path, test.name, configuration ):
            # An unexpected error occurred
            log.e( log.red + test.name + log.reset + ':', configuration_str( configuration, suffix = ' ' ) + 'exited with non-zero value (' + str(cpe.returncode) + ')' )


# Run all tests
if pyrs:
    sys.path.append( pyrs_path )
from rspy import devices
devices.query()
#
# Under Travis, we'll have no devices and no acroname
skip_live_tests = len(devices.all()) == 0  and  not devices.acroname
#
log.reset_errors()
tags = set()
tests = []
for test in prioritize_tests( get_tests() ):
    #
    log.d( 'found', test.name, '...' )
    try:
        log.debug_indent()
        test.debug_dump()
        #
        if tag and tag not in test.config.tags:
            log.d( 'does not fit --tag:', test.config.tags )
            continue
        #
        tags.update( test.config.tags )
        #
        tests.append( test.name )
        #
        if list_tags or list_tests:
            continue
        if not test.is_live():
            test_wrapper( test )
            continue
        #
        if skip_live_tests:
            log.w( test.name + ':', 'is live and there are no cameras; skipping' )
            continue
        #
        for configuration, serial_numbers in devices_by_test_config( test ):
            try:
                log.d( 'configuration:', configuration )
                log.debug_indent()
                devices.enable_only( serial_numbers, recycle = True )
            except RuntimeError as e:
                log.w( log.red + test.name + log.reset + ': ' + str(e) )
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
    log.e( 'No unit-tests found!' )
    sys.exit(1)
#
if list_tags or list_tests:
    if list_tags:
        print( "Available tags:" )
        for t in sorted( list( tags )):
            print( t )
    #
    if list_tests:
        print( "Available tests:" )
        for t in sorted( tests ):
            print( t )
#
else:
    n_errors = log.n_errors()
    if n_errors:
        log.out( log.red + str(n_errors) + log.reset, 'of', n_tests, 'test(s)', log.red + 'failed!' + log.reset + log.clear_eos )
        sys.exit(1)
    #
    log.out( str(n_tests) + ' unit-test(s) completed successfully' + log.clear_eos )
#
sys.exit(0)
