#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# $1 = path to vcpkg installation directory

set -e

rm -rf $1
mkdir $1
cd $1
git clone https://github.com/microsoft/vcpkg
cd vcpkg
git checkout 2021.05.12
./bootstrap-vcpkg.sh
./vcpkg integrate install

./vcpkg install ms-gsl
./vcpkg install boost-uuid
./vcpkg install boost-program-options
./vcpkg install gtest

