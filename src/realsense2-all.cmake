# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

if( NOT BUILD_SHARED_LIBS )

# This takes all the static libraries and combines them into a single one:
#     librealsense2-all.a

# modified from https://stackoverflow.com/questions/54269654/cmake-find-program-command-does-not-find-lib-exe
function( bundle_static_library target_name bundle_name )

    function( _recursively_collect_dependencies input_target )
        #message( "---> ${input_target}" )
        #list( APPEND CMAKE_MESSAGE_INDENT "  " )
        set( _input_link_libraries LINK_LIBRARIES )
        get_target_property( _input_type ${input_target} TYPE )
        if( ${_input_type} STREQUAL "INTERFACE_LIBRARY" )
            set( _input_link_libraries INTERFACE_LINK_LIBRARIES )
        endif()
        get_target_property( public_dependencies ${input_target} ${_input_link_libraries} )
        if( NOT public_dependencies STREQUAL "public_dependencies-NOTFOUND" )
            foreach( dependency IN LISTS public_dependencies )
                if( TARGET ${dependency} )
                    get_target_property(alias ${dependency} ALIASED_TARGET)
                    if( TARGET ${alias} )
                        set( dependency ${alias} )
                    endif()
                    get_target_property( _type ${dependency} TYPE )
                    if( ${_type} STREQUAL "STATIC_LIBRARY" )
                        list( APPEND static_libs ${dependency} )
                    else()
                        #message( "   x ${dependency} not a STATIC_LIBRARY (${_type})" )
                    endif()

                    get_property( library_already_added
                        GLOBAL PROPERTY _${target_name}_static_bundle_${dependency} )
                    if( NOT library_already_added )
                        set_property( GLOBAL PROPERTY _${target_name}_static_bundle_${dependency} ON )
                        _recursively_collect_dependencies( ${dependency} )
                    else()
                        #message( "   x ${dependency} already done" )
                    endif()
                else()
                    #message( "   x ${dependency} not a target" )
                endif()
            endforeach()
        endif()
        set( static_libs ${static_libs} PARENT_SCOPE )
    endfunction()

    list( APPEND static_libs ${target_name} )
    _recursively_collect_dependencies( ${target_name} )
    list( REMOVE_DUPLICATES static_libs )
    #message( "---> Dependencies for ${bundle_name}: ${static_libs}" )

    if( CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$" )

        set( bundle_path
                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundle_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" )

        file( WRITE ${CMAKE_BINARY_DIR}/${bundle_name}.ar.in
            "CREATE ${bundle_path}\n" )
        
        foreach( tgt IN LISTS static_libs )
            file( APPEND ${CMAKE_BINARY_DIR}/${bundle_name}.ar.in
                "ADDLIB $<TARGET_FILE:${tgt}>\n" )
        endforeach()
    
        file( APPEND ${CMAKE_BINARY_DIR}/${bundle_name}.ar.in "SAVE\n" )
        file( APPEND ${CMAKE_BINARY_DIR}/${bundle_name}.ar.in "END\n" )

        file( GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/${bundle_name}.ar
            INPUT ${CMAKE_BINARY_DIR}/${bundle_name}.ar.in )

        set( ar_tool ${CMAKE_AR} )
        if( CMAKE_INTERPROCEDURAL_OPTIMIZATION )
            # This is link-time-optimization (LTO): the archive tool can be something else,
            # depending on the compiler. But "CMake doesn't seem to honor CMAKE_AR when
            # CMAKE_INTERPROCEDURAL_OPTIMIZATION is set to TRUE"
            set( ar_tool ${CMAKE_CXX_COMPILER_AR} )
        endif()

        add_custom_command(
            COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundle_name}.ar
            DEPENDS ${static_libs}
            OUTPUT ${bundle_path}
            COMMENT "Bundling ${bundle_path} (with ${static_libs})"
            VERBATIM )

    elseif( MSVC )

        set( bundle_path  # NOTE: generator expression output in the COMMENT below only works with CMake 3.26
                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<CONFIG>/${CMAKE_STATIC_LIBRARY_PREFIX}${bundle_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" )

        foreach( tgt IN LISTS static_libs )
            list( APPEND static_libs_full_names $<TARGET_FILE:${tgt}> )
        endforeach()

        # /IGNORE:4006 - "symbol already defined in <object>; second definition ignored"
        #       happens because rsutils includes easylogging++, but so does realsense2
        add_custom_command(
            COMMAND ${CMAKE_AR} /NOLOGO /IGNORE:4006 /OUT:${bundle_path} ${static_libs_full_names}
            DEPENDS ${static_libs}
            OUTPUT ${bundle_path}
            COMMENT "Bundling ${bundle_path} (with ${static_libs})"
            VERBATIM )

    else()
        message( WARNING "Unknown compiler setup; realsense2-all will not be available" )
        return()
    endif()

    add_custom_target( ${bundle_name} ALL DEPENDS ${bundle_path} )
    add_dependencies( ${bundle_name} ${target_name} )
    set_target_properties( ${bundle_name} PROPERTIES FOLDER Library )

endfunction()

bundle_static_library( ${LRS_TARGET} ${LRS_TARGET}-all )

endif()  # ! BUILD_SHARED_LIBS
