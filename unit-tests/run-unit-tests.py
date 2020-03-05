#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, locale, re, platform, getopt

def usage():
    ourname = os.path.basename(sys.argv[0])
    print( 'Syntax: ' + ourname + ' <dir>' )
    print( '        Run all unit-tests in $dir (in Windows: .../build/Release; in Linux: .../build' )
    print( '        Test logs are kept in <dir>/unit-tests/<test-name>.log' )
    print( 'Options:' )
    print( '        --debug        Turn on debugging information' )
    print( '        -v, --verbose  Errors will dump the log to stdout' )
    print( '        -q, --quiet    Suppress output; rely on exit status (0=no failures)' )
    sys.exit(2)
def debug(*args):
    pass


# Set up the default output system; if not a terminal, disable colors!
if sys.stdout.isatty():
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
    red = reset = cr = clear_eos = ''
    def out(*args):
        print( *args )
    def progress(*args):
        debug( *args )

n_errors = 0
def error(*args):
    global red
    global reset
    out( red + '-E-' + reset, *args )
    global n_errors
    n_errors = n_errors + 1
def info(*args):
    out( '-I-', *args)


# Parse command-line:
try:
    opts,args = getopt.getopt( sys.argv[1:], 'hvq',
        longopts = [ 'help', 'verbose', 'debug', 'quiet' ])
except getopt.GetoptError as err:
    error( err )   # something like "option -a not recognized"
    usage()
verbose = False
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
if len(args) != 1:
    usage()
dir = args[0]
if not os.path.isdir( dir ):
    error( 'Directory not found:', dir )
    sys.exit(1)


def run(cmd, errorstatus=256, stdout=subprocess.PIPE):
    debug( 'Running:', cmd )
    handle = None
    try:
        if stdout != subprocess.PIPE:
            handle = open( stdout, "w" )
            stdout = handle
        rv = subprocess.run( cmd,
                             stdout = stdout,
                             stderr = subprocess.STDOUT,
                             universal_newlines = True )
        status = rv.returncode
        if status >= errorstatus:
            raise RuntimeError( 'status ' + str(status) + ' from ' + cmd )
        result = rv.stdout
        if not result:
            result = []
        else:
            result = result.split( '\n' )
        return status, result
    finally:
        if handle:
            handle.close()

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


logdir = dir + '/unit-tests'
os.makedirs( logdir, exist_ok = True );
n_tests = 0

# In Linux, the build targets are located elsewhere than on Windows
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

# Go over all the tests from a "manifest" we take from the result of the last CMake
# run (rather than, for example, looking for test-* in the build-directory):
if linux:
    manifestfile = dir + '/CMakeFiles/TargetDirectories.txt'
else:
    manifestfile = dir + '/../CMakeFiles/TargetDirectories.txt'
for manifest_ctx in grep( r'(?<=unit-tests/build/)\S+(?=/CMakeFiles/test-\S+.dir$)', manifestfile ):

    testdir = manifest_ctx['match'].group(0)                    # "log/internal/test-all"
    testparent = os.path.dirname(testdir)                       # "log/internal"
    testname = 'test-' + testparent.replace( '/', '-' ) + '-' + os.path.basename(testdir)[5:]   # "test-log-internal-all"
    if linux:
        exe = dir + '/unit-tests/build/' + testdir + '/' + testname
    else:
        exe = dir + '/' + testname + '.exe'
    log = logdir + '/' + testname + '.log'

    progress( testname, '>', log, '...' )
    n_tests += 1
    try:
        run( [exe], stdout=log )
    except FileNotFoundError:
        error( red + testname + reset + ': executable not found! (' + exe + ')' )
        continue

    # Normal logs are expected to have in last line:
    #     "All tests passed (11 assertions in 1 test case)"
    # Tests that have failures, however, will show:
    #     "test cases: 1 | 1 failed
    #      assertions: 9 | 6 passed | 3 failed"
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

progress()
if n_errors:
    out( red + str(n_errors) + reset + ' of ' + str(n_tests) + ' test(s) failed!' + clear_eos )
    sys.exit(1)
out( str(n_tests) + ' unit-test(s) completed successfully' + clear_eos )
sys.exit(0)
