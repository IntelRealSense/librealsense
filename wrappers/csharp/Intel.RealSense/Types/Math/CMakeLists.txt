target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/Vector.cs"
        "${CMAKE_CURRENT_LIST_DIR}/Quaternion.cs"
        "${CMAKE_CURRENT_LIST_DIR}/Vertex.cs"
        "${CMAKE_CURRENT_LIST_DIR}/TextureCoordinate.cs"
)
