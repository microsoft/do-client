#! /bin/bash

# bootstrap scripts will exit immediately if a command exits with a non-zero status
set -e

# Defaults 
COMMAND=all

usage() {
    cat <<EOM
$(basename $0) - Script to setup development environments for Delivery Optimization
Usage: $(basename $0) --platform <platform> --command <command>
	--platform			# Platform to provision, supported platforms: Ubuntu18.04, Ubuntu 20.04, Debian9, Debian 10. Default is Ubuntu18.04
	--command 			# Which command to run, supported commands: build, developertools, containertools, qemu, all. Default is All
EOM
    exit 1
}

function parseArgs() {
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
			if [[ "$PLATFORM" == "debian9" || "$PLATFORM" == "debian10" || "$PLATFORM" == "ubuntu18.04" || "$PLATFORM" == "ubuntu20.04" ]];
			then
				echo -e "[INFO] Platform set to: ${PLATFORM}"
			else 
				echo -e "[ERROR] Unsupported platform: ${PLATFORM}"
				exit
			fi
			
			shift
			;;
		--command | -c)
			COMMAND="${2,,}"
			echo -e "[INFO] Command to run set to: ${COMMAND}"
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
    apt-get install -y build-essential g++ gdb gdbserver git wget
    apt-get install -y python3 cmake ninja-build rpm

    # Open-source library dependencies
    # Boost libs for DO
    apt-get install -y libboost-system-dev libboost-log-dev libboost-filesystem-dev libboost-program-options-dev
    # Additional Boost libs for cpprestsdk
    apt-get install -y libboost-random-dev libboost-regex-dev
    apt-get install -y libgtest-dev libproxy-dev libmsgsl-dev libssl-dev uuid-dev

    # Install cpprest dependencies
    # libssl-dev also required but installed above because plugin uses libssl-dev directly
    apt-get install -y zlib1g-dev

    # Cpprestsdk 2.10.2 is the latest publicly available version on Ubuntu 18.04
    # Build and install v2.10.16 as it's the earliest version which supports url-redirection
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

    # libgtest-dev is a source package and requires manual installation
    mkdir /tmp/build_gtest/
    cd /tmp/build_gtest
    cmake /usr/src/gtest
    make
    make install
}

function installDeveloperTools
{
    echo "[INFO] Installing developer tools"
    apt install -y python-pip
    pip install cpplint

    # Installs to a non-standard location so add to PATH manually
    export PATH=$PATH:~/.local/bin
}

function installContainerTools
{
    echo "[INFO] Installing Docker"
    # Install docker to enable building cross-arch for arm
    # Instructions located at: https://docs.docker.com/engine/install/ubuntu/
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
}

function installQemu
{
    echo "[INFO] Installing Qemu for cross-arch support"
    # Install qemu for cross-arch support
    apt-get -y install qemu binfmt-support qemu-user-static

    # Register qemu with docker to more easily run cross-arch containers
    docker run --rm --privileged multiarch/qemu-user-static --reset -p yes 
}

function installAll
{
	echo "Setting up development environment for do-client"
    installBuildDependencies
    installDeveloperTools
    installContainerTools
    installQemu
}

main()
{
    parseArgs "$@"
	
	if [[ -v PLATFORM ]]
	then
		echo "[INFO] Platform check susccesful"
	else
		echo "[ERROR] Please set a valid target platform"
		exit
	fi
	
	echo "[INFO] Updating package manager"
    #apt-get update
	
	echo "[INFO] Running command: $COMMAND"
	case $COMMAND in 
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

