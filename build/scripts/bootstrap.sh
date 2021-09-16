#! /bin/bash
# Note: /bin/bash on macOS is very old, this script needs atleast bash 4 to be run
# bash 4 can be installed to the user directory via homebrew on mac
# And can be run as '/usr/local/bin/bash ./bootstrap.sh'

###
# This script handles provisioning of build environments for the Delivery Optimization client components on supported platforms
###

# Treat all command failures as fatal'
set -e

# Defaults
INSTALL=all
PLATFORM=unknown

function usage {
    cat <<EOM
$(basename $0) - Script to setup development environments for Delivery Optimization
Usage: $(basename $0) --platform <platform to install for> --install <install command>
    --platform          # Platform to provision, supported platforms: ubuntu1804, ubuntu2004, debian9, debian 10, osx. Required
    --install           # Which command to run, supported commands: build, developertools, containertools, qemu, all. Default is all
EOM
    exit 1
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
        --platform | -p)
            PLATFORM="${2,,}"
            if [[ "$PLATFORM" == "debian9" || "$PLATFORM" == "debian10" || "$PLATFORM" == "ubuntu1804" || "$PLATFORM" == "ubuntu2004" || "$PLATFORM" == "osx" ]];
            then
                echo -e "[INFO] Platform set to: ${PLATFORM}"
            else
                echo -e "[ERROR] Unsupported platform: ${PLATFORM}"
                exit 1
            fi

            shift
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
    if [[ $"PLATFORM" != "unknown" ]]
    then
        echo "[INFO] Platform check succesful"
    else
        echo "[WARNING] No platform supplied, please supply a platform."
        exit 1
    fi

    echo "[INFO] Installing build dependencies"

    if [[ "$PLATFORM" == "osx" ]];
    then
        # Need to install homebrew to get all the above stuff
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        brew install cmake ninja g++ gdb gdbserver python3 git curl unzip tar pkg-config

        ./install-vcpkg-deps.sh ~/deliveryoptimization_tools/
    elif isSupportedLinux
    then
        
        apt-get install -y make build-essential g++ gdb gdbserver gcc git wget
        apt-get install -y python3 ninja-build

        if [[ "$PLATFORM" == "debian9" ]];
        then
            # Cpprestsdk below requires min cmake version of 3.9, while 3.7 is the latest available on Debian9
            # So build & install cmake from source
            cd /tmp
            wget https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
            tar xzf cmake-3.10.2.tar.gz
            cd /tmp/cmake-3.10.2
            ./bootstrap
            make
            make install

            # Install gsl from source, also not available on Debian9
            cd /tmp/
            git clone https://github.com/Microsoft/GSL.git
            cd GSL/
            git checkout tags/v2.0.0
            cmake -DGSL_TEST=OFF .
            make
            make install
        else
            apt-get -y install cmake libmsgsl-dev
        fi

        # Open-source library dependencies
        # Boost libs for DO
        apt-get install -y libboost-system-dev libboost-filesystem-dev libboost-program-options-dev
        # Additional Boost libs for cpprestsdk
        apt-get install -y libboost-random-dev libboost-regex-dev
        apt-get install -y libproxy-dev libssl-dev uuid-dev

        # Install cpprest dependencies
        # libssl-dev also required but installed above because plugin uses libssl-dev directly
        apt-get install -y zlib1g-dev

        # Most target platforms do not natively have a version of cpprest that supports url-redirection
        # Build and install v2.10.16 as it's the earliest version which supports url-redirection
        rm -rf /tmp/cpprestsdk
        mkdir /tmp/cpprestsdk
        cd /tmp/cpprestsdk
        git clone https://github.com/microsoft/cpprestsdk.git .
        git checkout tags/v2.10.16
        git submodule update --init
        mkdir /tmp/cpprestsdk/build
        cd /tmp/cpprestsdk/build
        cmake -G Ninja -DCMAKE_BUILD_TYPE=minsizerel -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF -Wno-dev -DWERROR=OFF ..
        ninja
        ninja install

        rm -rf /tmp/gtest
        mkdir /tmp/gtest
        cd /tmp/gtest

        if [[ "$PLATFORM" == "ubuntu2004" || "$PLATFORM" == "debian10" ]];
        then
            # The latest native-version of gtest on ubuntu2004 and debian10 currently has a bug where CMakeLists doesn't declare an install target, causing 'make install' to fail
            # Clone from source and use release-1.10.0 instead, since gtest is a source package anyways
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
    else
        echo "[INFO] Builds not supported on this platform, no dependencies installed"
    fi
}

function installDeveloperTools
{
    echo "[INFO] Installing developer tools"
    if [[ "$PLATFORM" == "osx" ]]
    then
        brew install cpplint
    elif isSupportedLinux
    then
        apt-get install -y python-pip
        pip install cpplint

        # Installs to a non-standard location so add to PATH manually
        export PATH=$PATH:~/.local/bin
    else
        "[INFO] Developer tools not supported on this platform"
    fi
}

function installContainerTools
{
    if !isSupportedLinux
    then
        echo "[INFO] Container builds not supported on this platform"
    else
        apt-get install -y curl

        echo "[INFO] Installing Docker"
        # Install docker to enable building cross-arch for arm
        # Instructions located at: https://docs.docker.com/engine/install/ubuntu/
        curl -fsSL https://get.docker.com -o get-docker.sh
        sh get-docker.sh
    fi
}

function installQemu
{
    if !isSupportedLinux
    then
        echo "[INFO] Emulated builds not supported on this platform"
    else
        echo "[INFO] Installing Qemu for cross-arch support"
        # Install qemu for cross-arch support
        apt-get -y install qemu binfmt-support qemu-user-static

        # Register qemu with docker to more easily run cross-arch containers
        docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
    fi
}

function installAll
{
    echo "Setting up development environment for do-client"

    installBuildDependencies

    if isSupportedLinux
    then
        installDeveloperTools
        installContainerTools
        installQemu
    fi
}

function isSupportedLinux()
{
    if [[ "$PLATFORM" == "debian9" || "$PLATFORM" == "debian10" || "$PLATFORM" == "ubuntu1804" || "$PLATFORM" == "ubuntu2004" ]];
    then
        return 0
    else
        return 1
    fi
}

main()
{
    parseArgs "$@"

    if isSupportedLinux
    then
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
