# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 RealSense, Inc. All Rights Reserved.

# test:device D585S
# Currently testing with D585S because this is the only module supporting some of the features like module verbosity and version verification.
# When more D500 models will be available most of the test cases can be generelized, but D585S version verification should still be checked specifically. 

import pyrealsense2 as rs
from rspy import log, test
import os
import time

dev, _ = test.find_first_device_or_exit()
logger = rs.firmware_logger(dev)

fw_version = dev.get_info( rs.camera_info.firmware_version )
smcu_version = dev.get_info( rs.camera_info.smcu_fw_version )

with test.closure( 'test empty XML' ):
    empty_xml = ""
    try:
        logger.init_parser(empty_xml)
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Cannot find XML root" )

with test.closure( 'test root is not Format' ):
    xml = """<Source id="0" Name="test" />"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "XML root should be 'Format'" )

with test.closure( 'test source' ):
    xml = """<Format>
               <Source/>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Can't find attribute 'id' in node Source" )
        
    xml = """<Format>
               <Source id="0"/>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Can't find attribute 'Name' in node Source" )

    xml = """<Format>
               <Source id="3" Name="invalid" />
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Supporting source id 0 to 2. Found source (3, invalid)" )
    
    xml = """<Format>
               <Source id="-1" Name="invalid" />
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Supporting source id 0 to 2. Found source (-1, invalid)" )

with test.closure( 'test module' ):
    events_xml = """<Format/>"""
    events_file = open( "events.xml", "w" )
    events_file.write( events_xml )
    events_file.close()
    
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="32" />
               </Source>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Can't find attribute 'verbosity' in node Module" )
        
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="32" verbosity="0" />
               </Source>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Supporting module id 0 to 31. Found module 32 in source (0, test)" )
        
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="-1" verbosity="0" />
               </Source>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Supporting module id 0 to 31. Found module -1 in source (0, test)" )

with test.closure( 'test version verification' ):
    # Bad HKR and SMCU versions
    events_xml = "<Format version=\'1.2\'/>"
    events_file = open( "events.xml", "w" )
    events_file.write( events_xml )
    events_file.close()
    
    events2_xml = "<Format version=\'3.4\'/>"
    events2_file = open( "events2.xml", "w" )
    events2_file.write( events2_xml )
    events2_file.close()

    xml = """<Format>
               <Source id="0" Name="HKR" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
               <Source id="1" Name="SMCU" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""

    expected_error = "Source HKR expected version " + fw_version + " but xml file version is 1.2"
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, expected_error )

    # Fix HKR version, fail on SMCU
    events_xml = "<Format version=\'" + fw_version + "\'/>"
    events_file = open( "events.xml", "w" )
    events_file.write( events_xml )
    events_file.close()
    
    xml = """<Format>
               <Source id="0" Name="HKR" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
               <Source id="1" Name="SMCU" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""

    expected_error = "Source SMCU expected version " + smcu_version + " but xml file version is 3.4"
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, expected_error )

    # Both versions OK
    events2_xml = "<Format version=\'" + smcu_version + "\'/>"
    events2_file = open( "events2.xml", "w" )
    events2_file.write( events2_xml )
    events2_file.close()

    xml = """<Format>
               <Source id="0" Name="HKR" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
               <Source id="1" Name="SMCU" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""

    test.check( logger.init_parser( xml ) )

# Test after version verification, so event xml files will be ready with correct version.
with test.closure( 'test verbosity level' ):
    # Number is OK (range not checked)
    xml = """<Format>
               <Source id="0" Name="HKR" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="55" />
               </Source>
               <Source id="1" Name="SMCU" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""

    test.check( logger.init_parser( xml ) )
        
    # Starting with a digit but is not a number
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="0A" />
               </Source>
               <Source id="1" Name="test" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Bad verbosity level 0A" )
        
    # Valid verbosity keywords combined 
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="DEBUG|INFO|ERROR" />
               </Source>
               <Source id="1" Name="test" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="VERBOSE|FATAL" />
               </Source>
             </Format>"""
    test.check( logger.init_parser( xml ) )
        
    # Not one of the valid verbosity keywords
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="TEST" />
               </Source>
               <Source id="1" Name="test" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""
    try:
        logger.init_parser( xml )
    except RuntimeError as e:
        test.check_exception( e, RuntimeError, "Illegal verbosity TEST. Expecting NONE, VERBOSE, DEBUG, INFO, WARNING, ERROR or FATAL" )

# Test after version verification, so event xml files will be ready with correct version.
with test.closure( 'test module events file' ):        
    xml = """<Format>
               <Source id="0" Name="test" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="0" Path="events.xml" />
               </Source>
               <Source id="1" Name="test" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""
    test.check( logger.init_parser( xml ) )
    
with test.closure( 'test live log messages received' ):
    # Number is OK (range not checked)
    xml = """<Format>
               <Source id="0" Name="HKR" >
                 <File Path="events.xml" />
                 <Module id="0" verbosity="63" />
                 <Module id="1" verbosity="63" />
                 <Module id="2" verbosity="63" />
                 <Module id="3" verbosity="63" />
                 <Module id="4" verbosity="63" />
                 <Module id="5" verbosity="63" />
                 <Module id="6" verbosity="63" />
                 <Module id="7" verbosity="63" />
                 <Module id="8" verbosity="63" />
                 <Module id="9" verbosity="63" />
               </Source>
               <Source id="1" Name="SMCU" >
                 <File Path="events2.xml" />
                 <Module id="0" verbosity="0" />
               </Source>
             </Format>"""

    test.check( logger.init_parser( xml ) )
    logger.start_collecting()
    message = logger.create_message();
    for i in range(10):
        test.check( logger.get_firmware_log( message ) )
    logger.stop_collecting()
    
# Clean one time, not in every test case
if os.path.exists("events.xml"):
  os.remove("events.xml")
if os.path.exists("events2.xml"):
  os.remove("events2.xml")
 
test.print_results_and_exit()
