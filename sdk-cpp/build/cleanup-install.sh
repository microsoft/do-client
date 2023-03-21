#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Script to manually uninstall the SDK from the system.
# Use this when not using a package manager to install the SDK.

# This method deletes stuff so beware of the path provided!
remove_sdk() {
    if [ $1 == "" ]; then
        echo "Base path not provided"
    elif [ $2 == "" ]; then
        echo "sdk name not provided"
    else
        base_path=$1
        lib_name=$2
        echo "Removing from $base_path"
        rm $base_path/lib/lib$lib_name.*
        rm -r $base_path/lib/cmake/$lib_name/
        rm -r $base_path/include/$lib_name/
        rm $base_path/bin/docs
        rm -r $base_path/bin/docs-daemon/
        ls -lr $base_path/lib/ | grep *$lib_name*
        ls -l $base_path/include/ | grep $lib_name
    fi
}

remove_sdk /usr deliveryoptimization
remove_sdk /usr/local deliveryoptimization
# Remove cmake config files as well
remove_sdk /usr deliveryoptimization_sdk
remove_sdk /usr/local deliveryoptimization_sdk

# Also look for remnants using the older name
remove_sdk /usr ms-do-sdk
remove_sdk /usr/local ms-do-sdk
# Remove cmake config files as well
remove_sdk /usr ms_do_sdk
remove_sdk /usr/local ms_do_sdk
remove_sdk /usr dosdkcpp
remove_sdk /usr/local dosdkcpp
remove_sdk /usr docsdk
remove_sdk /usr/local docsdk
