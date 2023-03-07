#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -e

if [ $1 == "agent" -o $1 == "sdk-tests" ]
then
    echo "Building $1 snap..."

    if [ ! -d "./snap" ]
    then
        mkdir ./snap
    fi

    if [ $1 == "agent" ]
    then
        cp ./snapcraft-options/snapcraft-agent.yaml ./snap/snapcraft.yaml

    else
        cp ./snapcraft-options/snapcraft-sdk.yaml ./snap/snapcraft.yaml
    fi

    snapcraft

else
    echo "$1 is not a valid snap option, valid options are 'agent' or 'sdk-tests'"
fi