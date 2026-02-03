#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# $1 = path to source code
# $2 = debug or release

set -e

echo "Building apt plugin within Docker on Linux container"

echo "***********************************************"
echo "**** Building & Installing sdk from source ****"
echo "***********************************************"
cd $1
python3 build/build.py --project sdk --cmaketarget deliveryoptimization --config $2 --package-for DEB --clean
pushd "/tmp/build-deliveryoptimization-sdk/linux-$2"
echo "**** Installing sdk package ****"
dpkg --ignore-depends=deliveryoptimization-agent -i libdeliveryoptimization*.deb
popd

echo "***********************************************"
echo "**** Building linux-apt plugin from source ****"
echo "***********************************************"
python3 build/build.py --project plugin-apt --config $2 --package-for debian --clean

echo "***************************************"
echo "**** Installing the plugin package ****"
echo "***************************************"
pushd "/tmp/build-deliveryoptimization-plugin-apt/linux-$2/"
dpkg -i deliveryoptimization-plugin-apt*.deb
popd

echo "**** Removing packages ****"
dpkg -r libdeliveryoptimization-dev deliveryoptimization-plugin-apt libdeliveryoptimization
