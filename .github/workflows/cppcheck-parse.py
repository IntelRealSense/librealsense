# Parser for cppcheck_run.log files

import sys
def usage():
    print( 'usage: python chhpcheck-parse.py [OPTIONS] <path-to-log>' )
    print( '       --severity <W|E|l>    limit which severities are shown (Warnings, Errors, locations; default=WEl)' )
    sys.exit(1)

import getopt
try:
    opts, args = getopt.getopt( sys.argv[1:], 'h',
                                longopts=['help', 'severity='] )
except getopt.GetoptError as err:
    print( '-F-', err )  # something like "option -a not recognized"
    usage()
if len(args) != 1:
    usage()
logfile = args[0]

severities = 'WEl'
for opt, arg in opts:
    if opt in ('--severity'):
        severities = arg


class LineError( Exception ):
    def __init__( self, message, line=None ):
        global line_number, logfile
        if line is None:
            line = line_number
        super().__init__( f'{logfile}+{line}: {message}' )


handle = open( logfile, "r" )
line_number = 0
def nextline():
    global handle, line_number
    line_number += 1
    line = handle.readline()
    if not line:
        raise LineError( 'unexpected end-of-file' )
    return line.strip()

while True:
    line = nextline()
    if line == '<errors>':
        break

import re
while True:
    line = nextline()
    if line == '</errors>':
        break
    if not line.startswith( '<error ' ):
        raise LineError( 'unexpected line' )

    #print( f'-D-{line_number}- {line}' )
    e = dict( re.findall( '(\w+)="(.*?)"', line ) )
    severity = e['severity'][0].capitalize()
    id = e['id']
    cwe = e.get( 'cwe', '-' )
    file0 = e.get( 'file0', '' )
    msg = e['msg'].replace( '&apos;', "'" )

    line = nextline()
    printed = False
    while( line.startswith( '<location ' )):
        l = dict( re.findall( '(\w+)="(.*?)"', line ) )
        file = l['file']
        line = l['line']
        column = l['column']
        info = l.get( 'info', '' )
        if severity in severities:
            if 'l' in severities:
                if not printed:
                    print( f'{severity} {id:30} {file0:50} {msg}' )
                print( f'| {"-":30} {file+"+"+line+"."+column:50} {info}' )
            elif not printed:
                print( f'{severity} {id:30} {file+"+"+line:50} {msg}' )
        printed = True
        line = nextline()
    if not printed:  # no locations!?
        if severity in severities:
            print( f'{severity} {id:30} {file0 or file:50} {msg}' )

    if line != '</error>':
        raise LineError( 'unknown error line' )

sys.exit(0)
