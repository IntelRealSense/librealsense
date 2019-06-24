file(READ "config/99-realsense-libusb.rules" contents HEX)

set(UDEV_HEADER "${CMAKE_CURRENT_BINARY_DIR}/udev-rules.h")

file(WRITE "${UDEV_HEADER}" "#ifndef __UDEV_RULES_H__\n")
file(APPEND "${UDEV_HEADER}" "#define __UDEV_RULES_H__\n")
file(APPEND "${UDEV_HEADER}" "#ifdef __cplusplus\n")
file(APPEND "${UDEV_HEADER}" "extern \"C\" {\n")
file(APPEND "${UDEV_HEADER}" "#endif\n")

file(APPEND "${UDEV_HEADER}" "const char "
  "realsense_udev_rules[] = {")

string(LENGTH "${contents}" contents_length)
math(EXPR contents_length "${contents_length} - 1")

foreach(iter RANGE 0 ${contents_length} 2)
  string(SUBSTRING ${contents} ${iter} 2 line)
  file(APPEND "${UDEV_HEADER}" "0x${line},")
endforeach()

file(APPEND "${UDEV_HEADER}" "};\n")

file(APPEND "${UDEV_HEADER}" "#ifdef __cplusplus\n")
file(APPEND "${UDEV_HEADER}" "}\n")
file(APPEND "${UDEV_HEADER}" "#endif\n")

file(APPEND "${UDEV_HEADER}" "#endif//\n")
