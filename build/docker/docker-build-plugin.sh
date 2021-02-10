#! /bin/bash

# $1 = path to source code
# $2 = debug or release

set -e

echo "Building apt plugin within Docker on Linux container"

echo "Building & Installing sdk from source"
cd $1
python3 build/build.py --project sdk --cmaketarget deliveryoptimization --config $2 --generator "Unix Makefiles" --clean
cd /tmp/build-deliveryoptimization-sdk/linux-$2/
cmake --build . --target install

cd $1
echo "Building linux-apt plugin from source"
python3 build/build.py --project plugin-apt --config $2 --package-for debian --generator "Unix Makefiles" --clean

echo "Build of doclient-plugin completed"