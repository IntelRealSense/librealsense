#test:device D455
#test:donotrun:!linux

import pyrealsense2 as rs
from rspy import log, repo, test
from rspy.stopwatch import Stopwatch

#############################################################################################
#
test.start( "Run test-rest-api-wrapper test" )
import subprocess
run_time_stopwatch = Stopwatch()
run_time_threshold = 3
p = subprocess.run( ["pytest", "./wrappers/rest-api/tests/test_api_service.py"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                timeout=10,
                check=False )  # don't fail on errors

# Print failure information if subprocess failed
if p.returncode != 0:
    log.e(f"Subprocess failed with return code: {p.returncode}")
    if p.stdout:
        log.e("Subprocess output:")
        log.e(p.stdout)

test.check(p.returncode == 0) # verify success return code
run_time_seconds = run_time_stopwatch.get_elapsed()
if run_time_seconds > run_time_threshold:
    log.e('Time elapsed too high!', run_time_seconds, ' > ', run_time_threshold)
log.d("rs-enumerate-devices completed in:", run_time_seconds, "seconds")
test.check(run_time_seconds < run_time_threshold)
run_time_stopwatch.reset()

test.finish()
#
#############################################################################################
test.print_results_and_exit()