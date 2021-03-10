target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ErrorMarshaler.cs"
        "${CMAKE_CURRENT_LIST_DIR}/ObjectPool.cs"
        "${CMAKE_CURRENT_LIST_DIR}/Log.cs"
        "${CMAKE_CURRENT_LIST_DIR}/ICompositeDisposable.cs"
)
