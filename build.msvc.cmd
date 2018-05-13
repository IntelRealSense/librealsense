@echo off
mkdir build
cd build

mkdir msvc
cd msvc

cmake -G "NMake Makefiles" ../..
cmake --build .

cd ../..
