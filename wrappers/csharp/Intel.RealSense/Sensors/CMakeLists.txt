target_sources(${LRS_DOTNET_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/Sensor.cs"
        "${CMAKE_CURRENT_LIST_DIR}/SensorList.cs"
        "${CMAKE_CURRENT_LIST_DIR}/SoftwareSensor.cs"
        "${CMAKE_CURRENT_LIST_DIR}/PoseSensor.cs"
        "${CMAKE_CURRENT_LIST_DIR}/MaxUsableRangeSensor.cs"
)
