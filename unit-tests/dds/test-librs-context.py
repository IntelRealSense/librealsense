# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
log.nested = 'C  '

import pyrealsense2 as rs
if log.is_debug_on():
    rs.log_to_console( rs.log_severity.debug )


#############################################################################################
#
test.start( "Multiple participants on the same domain should fail" )
try:
    contexts = []
    contexts.append( rs.context( { 'dds': { 'enabled': True, 'domain': 124, 'participant': 'context1' }} ))
    # another context, same domain and name -> OK
    contexts.append( rs.context( { 'dds': { 'enabled': True, 'domain': 124, 'participant': 'context1' }} ))
    # without a name -> pick up the name from the existing participant (default is "librealsense")
    contexts.append( rs.context( { 'dds': { 'enabled': True, 'domain': 124 }} ))
    # same name, different domain -> different participant; should be OK:
    contexts.append( rs.context( { 'dds': { 'enabled': True, 'domain': 125, 'participant': 'context1' }} ))
    test.check_throws( lambda: rs.context( { 'dds': { 'enabled': True, 'domain': 124, 'participant': 'context2' }} ),
        RuntimeError, "A DDS participant 'context1' already exists in domain 124; cannot create 'context2'" )
except:
    test.unexpected_exception()
del contexts
test.finish()
#
#############################################################################################
test.print_results_and_exit()
