# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os, re, platform, subprocess, sys

from rspy import log

# get os and directories for future use
# NOTE: WSL will read as 'Linux' but the build is Windows-based!
system = platform.system()
if system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False


def inside_dir( root ):
    """
    Yield all files found in root, using relative names ('root/a' would be yielded as 'a')
    """
    for (path,subdirs,leafs) in os.walk( root ):
        for leaf in leafs:
            # We have to stick to Unix conventions because CMake on Windows is fubar...
            yield os.path.relpath( path + '/' + leaf, root ).replace( '\\', '/' )


def is_inside( file, directory ):
    """
    :param file: The file/directory we're checking
    :param directory: The parent directory

    :return: True if the file resides somewhere inside the directory (even several directories in)

    NOTE: A directory is considered inside itself! is_inside( dir, dir ) is True
    """
    directory = os.path.join( os.path.realpath( directory ), '' )
    file = os.path.realpath( file )

    # Return True if the common prefix of both is equal to directory
    # E.g. /a/b/c/d.rst and directory is /a/b, the common prefix is /a/b
    common = os.path.commonprefix( [file, directory] )
    return common == directory or os.path.join( common, '' ) == directory


def find( dir, mask ):
    """
    Yield all files in given directory (including sub-directories) that fit the given mask
    :param dir: directory in which to search
    :param mask: mask to compare file names to
    """
    pattern = re.compile( mask )
    for leaf in inside_dir( dir ):
        if pattern.search( leaf ):
            yield leaf

def is_executable(path_to_file):
    """
    :param path_to_file: path to a file
    :return: whether the file is an executable or not
    """
    global linux
    if linux:
        return os.access(path_to_file, os.X_OK)
    else:
        return path_to_file.endswith('.exe')

def remove_newlines (lines):
    for line in lines:
        if line[-1] == '\n':
            line = line[:-1]    # excluding the endline
        yield line

def _grep( pattern, lines, context ):
    """
    helper function for grep
    """
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
            matches += 1
    if matches:
        del context['index']
        del context['line']
        del context['match']

def grep( expr, *args ):
    pattern = re.compile( expr )
    context = dict()
    for filename in args:
        context['filename'] = filename
        with open( filename, errors = 'ignore' ) as file:
            for line in _grep( pattern, remove_newlines( file ), context ):
                yield line

def cat( filename ):
    with open( filename, errors = 'ignore' ) as file:
        for line in remove_newlines( file ):
            log.out( line )


def split_comments( filename, comment_delim_regex = '#' ):
    """
    Yields all lines in a file, but with comments separated:
        '  line'                yields ('  line', None )
        'line # comment at EOL' yields ('line', 'comment at EOL')
        '# comment line  '      yields ('', 'comment line')
    """
    context = dict()
    pattern = re.compile( r'^(.*?)(?:\s*' + comment_delim_regex + r'\s*(.*?)\s*)?$' )  # to end-of-line
    with open( filename, errors = 'ignore' ) as file:
        for line in remove_newlines( file ):
            match = pattern.search( line )
            line_without_comment = match.group(1)
            comment = match.group(2)                 # can be None
            yield (line_without_comment, comment)



