#! /bin/bash

# bootstrap scripts will exit immediately if a command exits with a non-zero status
set -e

echo "Setting up development environment for do-client"

# Various development machine tools 
apt-get update -y --fix-missing
apt-get install -y make build-essential g++ gdb gdbserver gcc git wget
apt-get install -y python3 cmake ninja-build

# Open-source library dependencies
apt-get install -y libboost-all-dev libgtest-dev libproxy-dev libmsgsl-dev libssl-dev uuid-dev

# Install cpprest dependencies
# libssl-dev also required but installed above because plugin uses libssl-dev directly
apt-get install -y zlib1g-dev

# Cpprestsdk 2.10.10 is the latest publicly available version on Debian 10
# Build and install v2.10.16 as it's the earliest version which supports url-redirection
mkdir /tmp/cpprestsdk
cd /tmp/cpprestsdk
git clone https://github.com/microsoft/cpprestsdk.git .
git checkout v2.10.16
git submodule update --init
mkdir /tmp/cpprestsdk/build
cd /tmp/cpprestsdk/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF -Wno-dev -DWERROR=OFF ..
ninja
ninja install

# libgtest-dev is a source package and requires manual installation
mkdir /tmp/build_gtest/
cd /tmp/build_gtest
cmake /usr/src/gtest
make
make install 

echo "Finished bootstrapping"