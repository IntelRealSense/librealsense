#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, locale, re, platform, getopt
from abc import ABC, abstractmethod

# Remove Python's default list of places to look for modules!
# We want only modules in the directories we specifically provide to be found,
# otherwise pyrs other than what we compiled might be found...
sys.path = list()
sys.path.append( '' )  # directs Python to search modules in the current directory first
# Add our py/ module directory
current_dir = os.path.dirname( os.path.abspath( __file__ ))
sys.path.append( current_dir + os.sep + "py" )

from rspy import log

def usage():
    ourname = os.path.basename(sys.argv[0])
    print( 'Syntax: ' + ourname + ' [options] [dir]' )
    print( '        dir: the directory holding the executable tests to run (default to the build directory')
    print( 'Options:' )
    print( '        --debug        Turn on debugging information' )
    print( '        -v, --verbose  Errors will dump the log to stdout' )
    print( '        -q, --quiet    Suppress output; rely on exit status (0=no failures)' )
    print( '        -r, --regex    run all tests that fit the following regular expression')
    print( '        -s, --stdout   do not redirect stdout to logs')
    sys.exit(2)


def filesin( root ):
    # Yield all files found in root, using relative names ('root/a' would be yielded as 'a')
    for (path,subdirs,leafs) in os.walk( root ):
        for leaf in leafs:
            # We have to stick to Unix conventions because CMake on Windows is fubar...
            yield os.path.relpath( path + '/' + leaf, root ).replace( '\\', '/' )

def find( dir, mask ):
    pattern = re.compile( mask )
    for leaf in filesin( dir ):
        if pattern.search( leaf ):
            #log.d(leaf)
            yield leaf

# get os and directories for future use
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

# this script is located in librealsense/unit-tests, so one directory up is the main repository
librealsense = os.path.dirname(current_dir)

# function for checking if a file is an executable

def is_executable(path_to_test):
    if linux:
        return os.access(path_to_test, os.X_OK)
    else:
        return path_to_test.endswith('.exe')

# Parse command-line:
try:
    opts,args = getopt.getopt( sys.argv[1:], 'hvqr:s',
        longopts = [ 'help', 'verbose', 'debug', 'quiet', 'regex=', 'stdout' ])
except getopt.GetoptError as err:
    log.e( err )   # something like "option -a not recognized"
    usage()
regex = None
to_stdout = False
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
    build = librealsense + os.sep + 'build'
    for executable in find(build, '(^|/)test-.*'):
        if not is_executable(executable):
            continue
        dir_with_test = build + os.sep + os.path.dirname(executable)
        if target and target != dir_with_test:
            log.e("Found executable tests in 2 directories:", target, "and", dir_with_test, ". Can't default to directory")
            usage()
        target = dir_with_test

if target:
    logdir = target + os.sep + 'unit-tests'
else: # no test executables were found. We put the logs directly in build directory
    logdir = librealsense + os.sep + 'build' + os.sep + 'unit-tests'
os.makedirs( logdir, exist_ok = True )
n_tests = 0

# wrapper function for subprocess.run
def subprocess_run(cmd, stdout = None):
    log.d( '    running:', cmd )
    handle = None
    try:
        if stdout  and  stdout != subprocess.PIPE:
            handle = open( stdout, "w" )
            stdout = handle
        rv = subprocess.run( cmd,
                             stdout = stdout,
                             stderr = subprocess.STDOUT,
                             universal_newlines = True,
                             check = True)
        result = rv.stdout
        if not result:
            result = []
        else:
            result = result.split( '\n' )
        return result
    finally:
        if handle:
            handle.close()

# Python scripts should be able to find the pyrealsense2 .pyd or else they won't work. We don't know
# if the user (Travis included) has pyrealsense2 installed but even if so, we want to use the one we compiled.
# we search the librealsense repository for the .pyd file (.so file in linux)
pyrs = ""
if linux:
    for so in find(librealsense, '(^|/)pyrealsense2.*\.so$'):
        pyrs = so
else:
    for pyd in find(librealsense, '(^|/)pyrealsense2.*\.pyd$'):
        pyrs = pyd

if pyrs:
    # After use of find, pyrs contains the path from librealsense to the pyrealsense that was found
    # We append it to the librealsense path to get an absolute path to the file to add to PYTHONPATH so it can be found by the tests
    pyrs_path = librealsense + os.sep + pyrs
    # We need to add the directory not the file itself
    pyrs_path = os.path.dirname(pyrs_path)
    log.d( 'found pyrealsense pyd in:', pyrs_path )

# Figure out which sys.path we want the tests to see, assuming we have Python tests
#     PYTHONPATH is what Python will ADD to sys.path for the child processes
# (We can simply change `sys.path` but any child python scripts won't see it; we change the environment instead)
#
# We also need to add the path to the python packages that the tests use
os.environ["PYTHONPATH"] = current_dir + os.sep + "py"
#
if pyrs:
    os.environ["PYTHONPATH"] += os.pathsep + pyrs_path

def remove_newlines (lines):
    for line in lines:
        if line[-1] == '\n':
            line = line[:-1]    # excluding the endline
        yield line

def grep_( pattern, lines, context ):
    index = 0
    matches = 0
    for line in lines:
        index = index + 1
        match = pattern.search( line )
        if match:
            context['index'] = index
            context['line']  = line
            context['match'] = match
            yield context
            matches = matches + 1
    if matches:
        del context['index']
        del context['line']
        del context['match']

def grep( expr, *args ):
    #log.d( f'grep {expr} {args}' )
    pattern = re.compile( expr )
    context = dict()
    for filename in args:
        context['filename'] = filename
        with open( filename, errors = 'ignore' ) as file:
            for line in grep_( pattern, remove_newlines( file ), context ):
                yield context

def cat( filename ):
    with open( filename, errors = 'ignore' ) as file:
        for line in remove_newlines( file ):
            log.out( line )

def check_log_for_fails( path_to_log, testname, exe ):
    # Normal logs are expected to have in last line:
    #     "All tests passed (11 assertions in 1 test case)"
    # Tests that have failures, however, will show:
    #     "test cases: 1 | 1 failed
    #      assertions: 9 | 6 passed | 3 failed"
    if path_to_log is None:
        return False
    for ctx in grep( r'^test cases:\s*(\d+) \|\s*(\d+) (passed|failed)', path_to_log ):
        m = ctx['match']
        total = int(m.group(1))
        passed = int(m.group(2))
        if m.group(3) == 'failed':
            # "test cases: 1 | 1 failed"
            passed = total - passed
        if passed < total:
            if total == 1  or  passed == 0:
                desc = 'failed'
            else:
                desc = str(total - passed) + ' of ' + str(total) + ' failed'

            if log.is_verbose_on():
                log.e( log.red + testname + log.reset + ': ' + desc )
                log.i( 'Executable:', exe )
                log.i( 'Log: >>>' )
                log.out()
                cat( path_to_log )
                log.out( '<<<' )
            else:
                log.e( log.red + testname + log.reset + ': ' + desc + '; see ' + path_to_log )
            return True
        return False


class TestConfig(ABC):  # Abstract Base Class
    """
    Configuration for a test, encompassing any metadata needed to control its run, like retries etc.
    """
    def __init__(self):
        self._configurations = list()

    @property
    def configurations(self):
        return self._configurations


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
        for context in grep( regex, source ):
            match = context['match']
            directive = match.group(1)
            params = [s for s in context['match'].group(2).split()]
            comment = match.group(3)
            if directive == 'device':
                log.d( '    configuration:', params )
                if not params:
                    log.e( source + str(context.index) + ': device directive with no devices listed' )
                else:
                    self._configurations.append( params )
            else:
                log.e( source + str(context.index) + ': invalid directive "' + directive + '"; ignoring' )


class Test(ABC):  # Abstract Base Class
    """
    Abstract class for a test. Holds the name of the test
    """
    def __init__(self, testname):
        log.d( 'found', testname )
        self._name = testname
        self._config = None

    @abstractmethod
    def run_test(self):
        pass

    @property
    def config(self):
        return self._config

    @property
    def name(self):
        return self._name

    def get_log( self ):
        global to_stdout
        if to_stdout:
            path = None
        else:
            path = logdir + os.sep + self.name + ".log"
        return path

    def is_live(self):
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
        log.d( '    script:', self.path_to_script )
        self._config = TestConfigFromText( self.path_to_script, r'#\s*test:' )

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
        if log.is_debug_on():
            cmd += ['--debug']
        if log.is_color_on():
            cmd += ['--color']
        return cmd

    def run_test( self ):
        log_path = self.get_log()
        try:
            subprocess_run( self.command, stdout=log_path )
        except FileNotFoundError:
            log.e( log.red + self.name + log.reset + ': executable not found! (' + self.path_to_script + ')' )
        except subprocess.CalledProcessError as cpe:
            if not check_log_for_fails(log_path, self.name, self.path_to_script):
                # An unexpected error occurred
                log.e( log.red + self.name + log.reset + ': exited with non-zero value! (' + str(cpe.returncode) + ')')


class ExeTest(Test):
    """
    Class for c/cpp tests. Hold the path to the executable for the test
    """
    def __init__( self, testname, exe ):
        """
        :param testname: name of the test
        :param exe: full path to executable
        """
        Test.__init__(self, testname)
        self.exe = exe

    @property
    def command(self):
        return [self.exe]

    def run_test( self ):
        log_path = self.get_log()
        try:
            subprocess_run( self.command, stdout=log_path )
        except FileNotFoundError:
            log.e( log.red + self.name + log.reset + ': executable not found! (' + self.exe + ')')
        except subprocess.CalledProcessError as cpe:
            if not check_log_for_fails( log_path, self.name, self.exe ):
                # An unexpected error occurred
                log.e( log.red + self.name + log.reset + ': exited with non-zero value! (' + str(cpe.returncode) + ')' )


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
        for manifest_ctx in grep(r'(?<=unit-tests/build/)\S+(?=/CMakeFiles/test-\S+.dir$)', manifestfile):
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
    for py_test in find(current_dir, '(^|/)test-.*\.py'):
        testparent = os.path.dirname(py_test)  # "log/internal" <-  "log/internal/test-all.py"
        if testparent:
            testname = 'test-' + testparent.replace('/', '-') + '-' + os.path.basename(py_test)[5:-3]  # remove .py
        else:
            testname = os.path.basename(py_test)[:-3]

        if regex and not pattern.search( testname ):
            continue

        yield PyTest(testname, py_test)


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
            log.e( log.red + test.name + log.reset + ': ' + str(e) )
            continue
        yield configuration, serial_numbers


log.i( 'Logs in:', logdir )
def test_wrapper( test, configuration = None ):
    global n_tests
    n_tests += 1
    if configuration:
        log.progress( '[' + ' '.join( configuration ) + ']', test.name, '...' )
    else:
        log.progress( test.name, '...' )
    test.run_test()


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
for test in get_tests():
    #
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
            devices.enable_only( serial_numbers, recycle = True )
        except RuntimeError as e:
            log.w( log.red + self.name + log.reset + ': ' + str(e) )
        else:
            test_wrapper( test, configuration )

log.progress()
#
if not n_tests:
    log.e( 'No unit-tests found!' )
    sys.exit(1)
#
n_errors = log.n_errors()
if n_errors:
    log.out( log.red + str(n_errors) + log.reset, 'of', n_tests, 'test(s)', log.red + 'failed!' + log.reset + log.clear_eos )
    sys.exit(1)
#
log.out( str(n_tests) + ' unit-test(s) completed successfully' + log.clear_eos )
#
sys.exit(0)
