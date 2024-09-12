cmake_minimum_required(VERSION 2.8.3...3.20.5)

set(HEADER_FILES_ROSLZ4
        ${CMAKE_CURRENT_LIST_DIR}/include/roslz4/lz4s.h
)

set(SOURCE_FILES_ROSLZ4
  ${CMAKE_CURRENT_LIST_DIR}/src/lz4s.c
  ${CMAKE_CURRENT_LIST_DIR}/src/xxhash.c
  ${CMAKE_CURRENT_LIST_DIR}/src/xxhash.h
)
