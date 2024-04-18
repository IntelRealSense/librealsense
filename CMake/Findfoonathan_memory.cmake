
# This is a dummy file, just so CMake does not complain:

# CMake Error at build/third-party/fastdds/CMakeLists.txt:239 (find_package):
#   By not providing "Findfoonathan_memory.cmake" in CMAKE_MODULE_PATH this
#   project has asked CMake to find a package configuration file provided by
#   "foonathan_memory", but CMake did not find one.
#
#   Could not find a package configuration file provided by "foonathan_memory"
#   with any of the following names:
#
#     foonathan_memoryConfig.cmake
#     foonathan_memory-config.cmake
#
#   Add the installation prefix of "foonathan_memory" to CMAKE_PREFIX_PATH or
#   set "foonathan_memory_DIR" to a directory containing one of the above
#   files.  If "foonathan_memory" provides a separate development package or
#   SDK, be sure it has been installed.

# FastDDS requires foonathan_memory and will not find it if we do not provide this
# file.
#
# Since all the variables should already be set by external_foonathan_memory.cmake,
# we don't really do anything.

# This may not be the proper way to do this. But it works...


#message( "In Findfoonathan_memory.cmake" )


#find_package( PkgConfig )
#pkg_check_modules( foonathan_memory QUIET foonathan_memory )


