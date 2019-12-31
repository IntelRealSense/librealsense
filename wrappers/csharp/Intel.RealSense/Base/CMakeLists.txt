target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/DeleterHandle.cs"
        "${CMAKE_CURRENT_LIST_DIR}/Object.cs"
        "${CMAKE_CURRENT_LIST_DIR}/PooledObject.cs"
	    "${CMAKE_CURRENT_LIST_DIR}/RefCountedPooledObject.cs"
)
