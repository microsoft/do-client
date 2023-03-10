#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Note: /bin/bash on macOS is very old, this script needs atleast bash 4 to be run
# bash 4 can be installed to the user directory via homebrew on mac
# And can be run as '/usr/local/bin/bash ./bootstrap.sh'

###
# This script handles provisioning of build environments for the Delivery Optimization client components on supported platforms
###

# Treat all command failures as fatal
set -e

# Defaults
INSTALL=all

# Distro and arch info
OS=""
VER=""
is_linux=false
is_macos=false
is_amd64=false
is_arm64=false
is_arm32=false

function usage {
    cat <<EOM
$(basename "$0") - Script to setup development environments for Delivery Optimization
Usage: $(basename "$0") --install <install command>
    --install           # Which command to run, supported commands: build, developertools, containertools, qemu, all. Default is all
EOM
}

function parseArgs {
    arg_Positional=()
    while [[ $# -gt 0 ]]; do
        case $1 in
        --help | -h)
            usage
            shift
            exit 0
            ;;
        --install | -i)
            INSTALL="${2,,}"
            echo -e "[INFO] Install command to run set to: ${INSTALL}"
            shift
            ;;
        *)
            arg_Positional+=("$1")
            shift
            ;;
        esac
    done
}

function installBuildDependencies
{
    echo "[INFO] Installing build dependencies"

    if [ "$is_macos" = true ];
    then
        # Need to install homebrew to get all the above stuff
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        brew install cmake ninja g++ gdb gdbserver python3 git curl unzip tar pkg-config

        ./install-vcpkg-deps.sh ~/deliveryoptimization_tools/
    elif [ "$is_linux" = true ];
    then

        apt-get install -y make build-essential g++ gdb gdbserver gcc git wget
        apt-get install -y python3 ninja-build
        apt-get install -y cmake libmsgsl-dev
        apt-get install -y libboost-system-dev libboost-filesystem-dev libboost-program-options-dev
        apt-get install -y libproxy-dev libssl-dev uuid-dev libcurl4-openssl-dev

        rm -rf /tmp/gtest
        mkdir /tmp/gtest
        pushd /tmp/gtest

        if [[ ($OS == "ubuntu" && $VER == "20.04") || ($OS == "debian" && $VER == "10") ]];
        then
            # The latest native-version of gtest on ubuntu2004 and debian10 currently has a bug where
            # CMakeLists doesn't declare an install target, causing 'make install' to fail.
            # Clone from source and use release-1.10.0 instead, since gtest is a source package anyways.
            git clone https://github.com/google/googletest.git .
            git checkout release-1.10.0
            mkdir cmake
            cd cmake
            cmake /tmp/gtest
            make
            make install
        else
            # libgtest-dev is a source package and requires manual installation
            apt-get -y install libgtest-dev
            cmake /usr/src/gtest
            make
            make install
        fi

        popd
        rm -rf /tmp/gtest

    else
        echo "[INFO] Builds not supported on this platform, no dependencies installed"
    fi
}

function installDeveloperTools
{
    echo "[INFO] Installing developer tools"
    if [ "$is_macos" = true ];
    then
        brew install cpplint
    elif [ "$is_linux" = true ];
    then
        apt-get install -y python-pip
        pip install cpplint

        # Installs to a non-standard location so add to PATH manually
        export PATH=$PATH:~/.local/bin
    else
        echo "[INFO] Developer tools not supported on this platform"
    fi
}

function installContainerTools
{
    if [ "$is_linux" = true ];
    then
        apt-get install -y curl

        echo "[INFO] Installing Docker"
        # Install docker to enable building cross-arch for arm
        # Instructions located at: https://docs.docker.com/engine/install/ubuntu/
        curl -fsSL https://get.docker.com -o get-docker.sh
        sh get-docker.sh
    else
        echo "[INFO] Container builds not supported on this platform"
    fi
}

function installQemu
{
    if [ "$is_linux" = true ];
    then
        echo "[INFO] Installing Qemu for cross-arch support"
        # Install qemu for cross-arch support
        apt-get -y install qemu binfmt-support qemu-user-static

        # Register qemu with docker to more easily run cross-arch containers
        docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
    else
        echo "[INFO] Emulated builds not supported on this platform"
    fi
}

function installAll
{
    echo "Setting up development environment for do-client"
    installBuildDependencies
    installDeveloperTools
    installContainerTools
    installQemu
}

function isSupportedLinux()
{
    if [[ ("$OS" == "ubuntu" && ($VER == "18.04" || $VER == "20.04"))
        || ("$OS" == "debian" && ($VER == "10" || $VER == "11")) ]];
    then
        return 0
    else
        return 1
    fi
}

function isSupportedMacOS()
{
    # No specific version check needed at present
    return 0
}

# From DU project: https://github.com/Azure/iot-hub-device-update/blob/main/scripts/install-deps.sh
function determine_machine_architecture()
{
    local arch=''
    arch="$(uname -m)"
    local ret_val=$?
    if [[ $ret_val != 0 ]]; then
        echo "Failed to get cpu architecture."
        return 1
    else
        if [[ $arch == aarch64* || $arch == armv8* ]]; then
            is_arm64=true
        elif [[ $arch == armv7* || $arch == 'arm' ]]; then
            is_arm32=true
        elif [[ $arch == 'x86_64' || $arch == 'amd64' ]]; then
            is_amd64=true
        else
            echo "Machine architecture '$arch' is not supported."
            return 1
        fi
    fi
}

function determine_linux_distro()
{
    # Checking distro name and version
    if [ -r /etc/os-release ]; then
        # freedesktop.org and systemd
        OS=$(grep "^ID\s*=\s*" /etc/os-release | sed -e "s/^ID\s*=\s*//")
        VER=$(grep "^VERSION_ID=" /etc/os-release | sed -e "s/^VERSION_ID=//")
        VER=$(sed -e 's/^"//' -e 's/"$//' <<< "$VER")
    elif type lsb_release > /dev/null 2>&1; then
        # linuxbase.org
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [ -f /etc/lsb-release ]; then
        # For some versions of Debian/Ubuntu without lsb_release command
        OS=$(grep DISTRIB_ID /etc/lsb-release | awk -F'=' '{ print $2 }')
        VER=$(grep DISTRIB_RELEASE /etc/lsb-release | awk -F'=' '{ print $2 }')
    elif [ -f /etc/debian_version ]; then
        # Older Debian/Ubuntu/etc.
        OS=Debian
        VER=$(cat /etc/debian_version)
    else
        # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
        OS=$(uname -s)
        VER=$(uname -r)
    fi

    # Convert OS to lowercase
    OS="$(echo "$OS" | tr '[:upper:]' '[:lower:]')"
}

function determine_osx_ver()
{
    echo "Not implemented yet"
    return 1
}

function determine_os_and_arch()
{
    if [[ $OSTYPE == linux* ]]; then
        echo "Running on a linux-based OS"
        is_linux=true
        determine_linux_distro || return 1
    elif [[ $OSTYPE == darwin* ]]; then
        echo "Running on MacOS"
        is_macos=true
        determine_osx_ver || return 1
    else
        echo "Unsupported OS '$OSTYPE'"
        return 1
    fi

    determine_machine_architecture || return 1
}

function isSupportedOS()
{
    if [ "$is_linux" = true ]; then
        if ! isSupportedLinux; then
            echo "Unsupported Linux distro/version"
            return 1
        fi
    elif [ "$is_macos" = true ]; then
        if ! isSupportedMacOS; then
            echo "Unsupported MacOS version"
            return 1
        fi
    else
        echo "Unsupported OS and/or Version"
        return 1
    fi
    return 0
}

main()
{
    determine_os_and_arch || return 1

    echo "OS = $OS"
    echo "VER = $VER"
    if [ "$is_amd64" = true ]; then echo "Arch = amd64"
    elif [ "$is_arm32" = true ]; then echo "Arch = arm32"
    elif [ "$is_arm64" = true ]; then echo "Arch = arm64"
    fi

    isSupportedOS || return 1

    parseArgs "$@"

    if [ "$is_linux" = true ]; then
        echo "[INFO] Updating package manager"
        apt-get update -y --fix-missing
    fi

    echo "[INFO] Running install command: $INSTALL"
    case $INSTALL in
    all)
        installAll
        ;;
    build)
        installBuildDependencies
        ;;
    developertools)
        installDeveloperTools
        ;;
    containertools)
        installContainerTools
        ;;
    qemu)
        installQemu
        ;;
    esac

    echo "[INFO] Finished bootstrapping"
}

main "$@"
