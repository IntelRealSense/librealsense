# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os, re, platform, subprocess, sys

sys.path = list()
sys.path.append( '' )  # directs Python to search modules in the current directory first
sys.path.append( os.path.dirname( sys.executable ))
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'DLLs' ))
sys.path.append( os.path.join( os.path.dirname( sys.executable ), 'lib' ))
# Add our py/ module directory
current_dir = os.path.dirname( os.path.abspath( __file__ ))
sys.path.append( os.path.dirname( current_dir ) )

from rspy import log

# get os and directories for future use
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

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

def is_executable(path_to_test):
    global linux
    if linux:
        return os.access(path_to_test, os.X_OK)
    else:
        return path_to_test.endswith('.exe')

# wrapper function for subprocess.run
def subprocess_run(cmd, stdout = None):
    log.d( 'running:', cmd )
    handle = None
    try:
        log.debug_indent()
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
        log.debug_unindent()

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
                yield line

def cat( filename ):
    with open( filename, errors = 'ignore' ) as file:
        for line in remove_newlines( file ):
            log.out( line )
