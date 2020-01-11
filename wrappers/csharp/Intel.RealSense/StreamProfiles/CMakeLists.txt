target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/StreamProfile.cs"
        "${CMAKE_CURRENT_LIST_DIR}/StreamProfileList.cs"
        "${CMAKE_CURRENT_LIST_DIR}/VideoStreamProfile.cs"
        "${CMAKE_CURRENT_LIST_DIR}/MotionStreamProfile.cs"
        "${CMAKE_CURRENT_LIST_DIR}/PoseStreamProfile.cs"
)
