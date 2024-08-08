
# Image files are not included in our distribution. Define a function for easy downloading at CMake time:
#realsense-hw-public/rs-tests/algo/depth-to-rgb-calibration/19.2.20/F9440687/Snapshots/LongRange 768X1024 (RGB 1920X1080)
set(ALGO_SRC_URL "https://librealsense.intel.com/rs-tests/algo")

function(dl_algo_file filename sha1)
    dl_file( ${ALGO_SRC_URL} algo ${filename} ${sha1} )
endfunction()

