#!/bin/bash -e
# Installs glfw3 from source as a shared lib

# Make sure glfw (or any previous version) is not already installed
sudo apt-get update

sudo apt-get install build-essential cmake git xorg-dev libglu1-mesa-dev

git clone https://github.com/glfw/glfw.git /tmp/glfw

cd /tmp/glfw

git checkout latest

cmake . -DBUILD_SHARED_LIBS=ON

make

sudo make install

sudo ldconfig

rm -rf /tmp/glfw

echo "Done installing glfw3!"