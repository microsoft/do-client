#! /bin/bash

# bootstrap scripts will exit immediately if a command exits with a non-zero status
set -e

echo "Setting up development environment for do-client"

# Various development machine tools
apt-get update -y --fix-missing
apt-get install -y make build-essential g++ gdb gdbserver gcc git wget

# Cpprestsdk below requires min cmake version of 3.9, while 3.7 is the latest available on Debian9
# So build & install cmake from source
cd /tmp
wget https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
tar xzf cmake-3.10.2.tar.gz
cd /tmp/cmake-3.10.2
./bootstrap
make
make install

apt-get install -y python3 ninja-build

# Open-source library dependencies
# Boost libs for DO
apt-get install -y libboost-system-dev libboost-log-dev libboost-filesystem-dev libboost-program-options-dev
# Additional Boost libs for cpprestsdk
apt-get install -y libboost-random-dev libboost-regex-dev
apt-get install -y libcpprest-dev libgtest-dev libproxy-dev libssl-dev uuid-dev

# Install cpprest dependencies
# libssl-dev also required but installed above because plugin uses libssl-dev directly
apt-get install -y zlib1g-dev

# Cpprestsdk 2.9.1 is the latest publicly available version on Debian 9
# Build and install v2.10.16 as it's the earliest version which supports url-redirection
mkdir /tmp/cpprestsdk
cd /tmp/cpprestsdk
git clone https://github.com/microsoft/cpprestsdk.git .
git checkout v2.10.16
git submodule update --init
mkdir /tmp/cpprestsdk/build
cd /tmp/cpprestsdk/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF -Wno-dev -DWERROR=OFF ..
ninja
ninja install

# libgtest-dev is a source package and requires manual installation
mkdir /tmp/build_gtest/
cd /tmp/build_gtest
cmake /usr/src/gtest
make
make install

# Install gsl from source (not present in debian stretch packages)
cd /tmp/
git clone https://github.com/Microsoft/GSL.git
cd GSL/
git checkout tags/v2.0.0
cmake -DGSL_TEST=OFF .
make
make install

echo "Finished bootstrapping"