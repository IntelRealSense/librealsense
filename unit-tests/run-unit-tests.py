#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, locale, re, platform, getopt
from abc import ABC, abstractmethod

# Add our py/ module directory to Python's list so we can use them
current_dir = os.path.dirname( os.path.abspath( __file__ ))
sys.path.insert( 1, current_dir + os.sep + "py" )

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
    print( '        -a, --asis     do not init acroname; just run the tests as-is' )
    sys.exit(2)
def debug(*args):
    pass

def stream_has_color( stream ):
    if not hasattr(stream, "isatty"):
        return False
    if not stream.isatty():
        return False # auto color only on TTYs
    try:
        import curses
        curses.setupterm()
        return curses.tigetnum( "colors" ) > 2
    except:
        # guess false in case of error
        return False

# Set up the default output system; if not a terminal, disable colors!
if stream_has_color( sys.stdout ):
    red = '\033[91m'
    gray = '\033[90m'
    reset = '\033[0m'
    cr = '\033[G'
    clear_eos = '\033[J'
    clear_eol = '\033[K'
    _progress = ''
    def out(*args):
        print( *args, end = clear_eol + '\n' )
        global _progress
        if len(_progress):
            progress( *_progress )
    def progress(*args):
        print( *args, end = clear_eol + '\r' )
        global _progress
        _progress = args
else:
    red = gray = reset = cr = clear_eos = ''
    def out(*args):
        print( *args )
    def progress(*args):
        print( *args )

n_errors = 0
def error(*args):
    global red
    global reset
    out( red + '-E-' + reset, *args )
    global n_errors
    n_errors = n_errors + 1
def info(*args):
    out( '-I-', *args)

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
            debug(leaf)
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
    opts,args = getopt.getopt( sys.argv[1:], 'hvqr:sa',
        longopts = [ 'help', 'verbose', 'debug', 'quiet', 'regex=', 'stdout', 'asis' ])
except getopt.GetoptError as err:
    error( err )   # something like "option -a not recognized"
    usage()
verbose = False
regex = None
to_stdout = False
asis = False
for opt,arg in opts:
    if opt in ('-h','--help'):
        usage()
    elif opt in ('--debug'):
        def debug(*args):
            global gray
            global reset
            out( gray + '-D-', *args, reset )
    elif opt in ('-v','--verbose'):
        verbose = True
    elif opt in ('-q','--quiet'):
        def out(*args):
            pass
    elif opt in ('-r', '--regex'):
        regex = arg
    elif opt in ('-s', '--stdout'):
        to_stdout = True
    elif opt in ('-a', '--asis'):
        asis = True

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
            error("Found executable tests in 2 directories:", target, "and", dir_with_test, ". Can't default to directory")
            usage()
        target = dir_with_test

if target:
    logdir = target + os.sep + 'unit-tests'
else: # no test executables were found. We put the logs directly in build directory
    logdir = librealsense + os.sep + 'build'
os.makedirs( logdir, exist_ok = True )
n_tests = 0

# wrapper function for subprocess.run
def subprocess_run(cmd, stdout = None):
    debug( 'Running:', cmd )
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
    # Add the necessary path to the PYTHONPATH environment variable so python will look for modules there
    os.environ["PYTHONPATH"] = pyrs_path
    # We also need to add the path to the python packages that the tests use
    os.environ["PYTHONPATH"] += os.pathsep + (current_dir + os.sep + "py")
    # We can simply change `sys.path` but any child python scripts won't see it. We change the environment instead.

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
    #debug( f'grep {expr} {args}' )
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
            out( line )

def check_log_for_fails(log, testname, exe):
    # Normal logs are expected to have in last line:
    #     "All tests passed (11 assertions in 1 test case)"
    # Tests that have failures, however, will show:
    #     "test cases: 1 | 1 failed
    #      assertions: 9 | 6 passed | 3 failed"
    if log is None:
        return False
    global verbose
    for ctx in grep( r'^test cases:\s*(\d+) \|\s*(\d+) (passed|failed)', log ):
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

            if verbose:
                error( red + testname + reset + ': ' + desc )
                info( 'Executable:', exe )
                info( 'Log: >>>' )
                out()
                cat( log )
                out( '<<<' )
            else:
                error( red + testname + reset + ': ' + desc + '; see ' + log )
            return True
        return False

# definition of classes for tests
class Test(ABC): # Abstract Base Class
    """
    Abstract class for a test. Holds the name of the test
    """
    def __init__(self, testname):
        self.testname = testname

    @abstractmethod
    def run_test(self):
        pass

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

    @property
    def command(self):
        return [sys.executable, self.path_to_script]

    def run_test(self):
        global n_tests, to_stdout
        n_tests += 1
        if to_stdout:
            log = None
        else:
            log = logdir + os.sep + self.testname + ".log"
        progress(self.testname, '>', log, '...')
        try:
            subprocess_run(self.command, stdout=log)
        except FileNotFoundError:
            error(red + self.testname + reset + ': executable not found! (' + self.path_to_script + ')')
        except subprocess.CalledProcessError as cpe:
            if not check_log_for_fails(log, self.testname, self.path_to_script):
                # An unexpected error occurred
                error(red + self.testname + reset + ': exited with non-zero value! (' + str(cpe.returncode) + ')')

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

    def run_test(self):
        global n_tests, to_stdout
        n_tests += 1
        if to_stdout:
            log = None
        else:
            log = logdir + os.sep + self.testname + ".log"
        progress( self.testname, '>', log, '...' )
        try:
            subprocess_run(self.command, stdout=log)
        except FileNotFoundError:
            error(red + self.testname + reset + ': executable not found! (' + self.exe + ')')
        except subprocess.CalledProcessError as cpe:
            if not check_log_for_fails(log, self.testname, self.exe):
                # An unexpected error occurred
                error(red + self.testname + reset + ': exited with non-zero value! (' + str(cpe.returncode) + ')')


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
        for manifest_ctx in grep(r'(?<=unit-tests/build/)\S+(?=/CMakeFiles/test-\S+.dir$)', manifestfile):
            # We need to first create the test name so we can see if it fits the regex
            testdir = manifest_ctx['match'].group(0)  # "log/internal/test-all"
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

    # If we run python tests with no .pyd/.so file they will crash. Therefore we only run them if such a file was found
    if pyrs:
        # unit-test scripts are in the same directory as this script
        for py_test in find(current_dir, '(^|/)test-.*\.py'):
            testparent = os.path.dirname(py_test)  # "log/internal" <-  "log/internal/test-all.py"
            if testparent:
                testname = 'test-' + testparent.replace('/', '-') + '-' + os.path.basename(py_test)[5:-3]  # remove .py
            else:
                testname = os.path.basename(py_test)[:-3]

            if regex and not pattern.search( testname ):
                continue

            yield PyTest(testname, py_test)

# Before we run any tests, recycle all ports and make sure they're set to USB3
if not asis:
    try:
        import acroname
        acroname.connect()
        acroname.enable_ports()     # so ports() will return all
        portlist = acroname.ports()
        debug( 'Recycling found ports:', portlist )
        acroname.set_ports_usb3( portlist )   # will recycle them, too
        acroname.disconnect()
        import time
        time.sleep(5)
    except ModuleNotFoundError:
        # Error should have already been printed
        # We assume there's no brainstem library, meaning no acroname either
        pass

# Run all tests
for test in get_tests():
    test.run_test()

progress()
if n_errors:
    out( red + str(n_errors) + reset + ' of ' + str(n_tests) + ' test(s) failed!' + clear_eos )
    sys.exit(1)
out( str(n_tests) + ' unit-test(s) completed successfully' + clear_eos )
sys.exit(0)
