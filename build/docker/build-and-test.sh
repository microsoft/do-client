#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -e

if [ $# -ne 3 ] && [ $# -ne 4 ]; then
   echo "Usage: $0 <source code path> <build configuration> <component names (comma-separated)>"
   echo "    <source code path> is the path to the root of the source code"
   echo "    <build configuration> can be: debug, release, minsizerel, relwithdebinfo"
   echo "    <component names> can be: all, agent, sdk, plugin-apt"
   exit 1
fi

# ---- variables ----

# Assign command line arguments to variables
SOURCE_PATH=$1
BUILD_CONFIG=$2
COMPONENTS=$3
RUN_TESTS=false
if [ $# -eq 4 ] && [ "$4" == "run_tests" ]; then
    RUN_TESTS=true
fi

BUILD_SCRIPT_PATH=$SOURCE_PATH/build/build.py
AGENT_BUILD_ROOT=/tmp/build-deliveryoptimization-agent/linux-$BUILD_CONFIG
SDK_BUILD_ROOT=/tmp/build-deliveryoptimization-sdk/linux-$BUILD_CONFIG
PLUGIN_APT_BUILD_ROOT=/tmp/build-deliveryoptimization-plugin-apt/linux-$BUILD_CONFIG

if [ $(id -u) -ne 0 ]; then
    SUDO="sudo"
fi


# ---- functions ----

# Build the agent component
function build_agent
{
    echo "Building agent component"
    python3 $BUILD_SCRIPT_PATH --project agent --config $BUILD_CONFIG --package-for DEB --clean

    if [ "$RUN_TESTS" = true ]; then
        install_agent
        echo "Running agent tests"
        $AGENT_BUILD_ROOT/client-lite/test/deliveryoptimization-agent-tests "--gtest_filter=-NetworkMonitorTests*"
    fi
}

function install_agent
{
    echo "Installing agent component"
    $SUDO dpkg -i $AGENT_BUILD_ROOT/deliveryoptimization-agent*.deb
}

# Build the sdk component
function build_sdk
{
    if [ ! -d "$AGENT_BUILD_ROOT" ]; then
        echo "Agent component not built yet"
        build_agent
    fi
    install_agent

    echo "Building sdk component"
    python3 $BUILD_SCRIPT_PATH --project sdk --cmaketarget deliveryoptimization --config $BUILD_CONFIG --package-for DEB --clean

    if [ "$RUN_TESTS" = true ]; then
        install_sdk

        echo "Building sdk tests"
        python3 $BUILD_SCRIPT_PATH --project sdk --cmaketarget deliveryoptimization-sdk-tests --config $BUILD_CONFIG

        echo "Running sdk tests"
        $SUDO $SDK_BUILD_ROOT/sdk-cpp/tests/deliveryoptimization-sdk-tests
    fi
}

function install_sdk
{
    echo "Installing sdk component"
    $SUDO dpkg -i $SDK_BUILD_ROOT/libdeliveryoptimization*.deb
}

# Build the plugin-apt component
function build_plugin_apt
{
    if [ ! -d "$SDK_BUILD_ROOT" ]; then
        echo "SDK component not built yet"
        build_sdk
    fi
    $SUDO dpkg --ignore-depends=deliveryoptimization-agent -i $SDK_BUILD_ROOT/libdeliveryoptimization*.deb

    echo "Building plugin-apt component"
    python3 $BUILD_SCRIPT_PATH --project plugin-apt --config $BUILD_CONFIG --package-for DEB --clean

    install_plugin_apt
}

function install_plugin_apt
{
    echo "Installing plugin-apt component"
    $SUDO dpkg -i $PLUGIN_APT_BUILD_ROOT/deliveryoptimization-plugin-apt*.deb
}

function build_all
{
    echo "Building all components"
    build_agent
    build_sdk
    build_plugin_apt
}

function uninstall_all
{
    echo "Uninstalling all components"
    $SUDO dpkg -r deliveryoptimization-plugin-apt 2>/dev/null
    $SUDO dpkg -r libdeliveryoptimization-dev 2>/dev/null
    $SUDO dpkg -r libdeliveryoptimization 2>/dev/null
    $SUDO dpkg -r deliveryoptimization-agent 2>/dev/null
}

# ---- main ----

pushd $SOURCE_PATH

echo "Will run tests? $RUN_TESTS."

# Split comma-separated components into an array
IFS=',' read -ra COMPONENT_ARRAY <<< "$COMPONENTS"

# Loop through the array and build the selected components
for COMPONENT in "${COMPONENT_ARRAY[@]}"; do
    case $COMPONENT in
        all)
            build_all
            ;;
        agent)
            build_agent
            ;;
        sdk)
            build_sdk
            ;;
        plugin-apt)
            build_plugin_apt
            ;;
        *)
            echo "Invalid component name: $COMPONENT"
            exit 2
            ;;
    esac
done

popd
