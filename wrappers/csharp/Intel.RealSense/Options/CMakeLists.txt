target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/IOption.cs"
        "${CMAKE_CURRENT_LIST_DIR}/IOptions.cs"
        "${CMAKE_CURRENT_LIST_DIR}/IOptionsContainer.cs"
        "${CMAKE_CURRENT_LIST_DIR}/OptionsList.cs"
        "${CMAKE_CURRENT_LIST_DIR}/OptionInternal.cs"
)
