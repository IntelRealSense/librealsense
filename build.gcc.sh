#!/bin/bash
mkdir build
cd build

mkdir gcc
cd gcc

cmake ../..
cmake --build .

cd ../..
