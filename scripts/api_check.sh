#!/bin/bash

# This script makes sure every API header can be included in isolation

for filename in $(find ../include/librealsense2 -name *.hpp); do
    echo Checking that $filename is self-contained
    rm 1.cpp
    echo "#include \"$filename\"" >> 1.cpp
    echo "int main() {}" >> 1.cpp
    g++ -std=c++11 1.cpp || exit 1
    echo $filename is OK!
    echo
done

for filename in $(find ../include/librealsense2 -name *.h); do
    echo Checking that $filename is self-contained
    rm 1.c
    echo "#include \"$filename\"" >> 1.c
    echo "int main() {}" >> 1.c
    g++ 1.c || exit 1
    echo $filename is OK!
    echo
done

rm 1.cpp
rm 1.c
