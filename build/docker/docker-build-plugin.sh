#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# $1 = path to source code
# $2 = debug or release

set -e

echo "Building apt plugin within Docker on Linux container"

echo "Building & Installing sdk from source"
cd $1
python3 build/build.py --project sdk --cmaketarget deliveryoptimization --config $2 --package-for DEB --clean
pushd "/tmp/build-deliveryoptimization-sdk/linux-$2"
dpkg --ignore-depends=deliveryoptimization-agent -i libdeliveryoptimization*.deb
popd

echo "Building linux-apt plugin from source"
python3 build/build.py --project plugin-apt --config $2 --package-for debian --clean

echo "Build of doclient-plugin completed"

echo "Installing the plugin package"
pushd "/tmp/build-deliveryoptimization-plugin-apt/linux-$2/"
dpkg -i deliveryoptimization-plugin-apt*.deb
popd

echo "Removing packages"
dpkg -r libdeliveryoptimization-dev deliveryoptimization-plugin-apt libdeliveryoptimization
