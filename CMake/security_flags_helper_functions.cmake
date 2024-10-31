macro(push_security_flags)  # remove security flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SECURITY_COMPILER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SECURITY_COMPILER_FLAGS}")
endmacro()

macro(pop_security_flags)  # append security flags
    string(REPLACE "${SECURITY_COMPILER_FLAGS}" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "${SECURITY_COMPILER_FLAGS}" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endmacro()

macro(set_security_flags_for_executable) # replace flag fPIC (Position-Independent Code) with fPIE (Position-Independent Executable)
    string(REPLACE "-fPIC" "-fPIE" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-fPIC" "-fPIE" CMAKE_C_FLAGS  "${CMAKE_C_FLAGS}")
endmacro()

macro(unset_security_flags_for_executable) # replace flag fPIE (Position-Independent Executable) with fPIC (Position-Independent Code)
    string(REPLACE "-fPIE" "-fPIC" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-fPIE" "-fPIC" CMAKE_C_FLAGS  "${CMAKE_C_FLAGS}")
endmacro()

