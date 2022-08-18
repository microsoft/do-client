# Dockerfile for building DO client components for Debian 10 arm64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-debian10-arm64

FROM mcr.microsoft.com/mirror/docker/library/debian:buster@sha256:e774922f3043b6564c6719a7ec32f25d4b70304eff9dbd725161d4033cf38038

SHELL [ "/bin/bash", "-c"]

# QEMU is a Linux emulator which enables cross-arch support in docker
# In order to build this image on a Linux host, need to install QEMU:
#
# sudo apt-get install qemu-user
# update-binfmts --display
# sudo apt install qemu binfmt-support qemu-user-static
# cp /usr/bin/qemu-arm-static <src root>/build/docker/debian10/arm64
#
# Then copy the build script to the build directory
# cp <src root>/build/scripts/bootstrap.sh <src root>/build/docker/debian10/arm64
#
# open bootstrap.sh and change the function installQemu to the following:
#
# function installQemu
# {
#        echo "[INFO] Installing Qemu for cross-arch support"
#        # Install qemu for cross-arch support
#        apt-get -y install qemu binfmt-support qemu-user-static
#
#        # Register qemu with docker to more easily run cross-arch containers
#       docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
# }
# After building the image yoou can return the bootstrap.sh file to the original state

# After running the above, you can build the image by running in the current dockerfile directory
# sudo docker build -t debian10_arm64 . --no-cache --network=host

COPY qemu-arm-static /usr/bin/qemu-arm-static
COPY bootstrap.sh /tmp/bootstrap.sh

WORKDIR /tmp/
RUN chmod +x bootstrap.sh
RUN ./bootstrap.sh --platform debian10 --install build

VOLUME /code
WORKDIR /code

ENTRYPOINT [ "/bin/bash", "-c" ]

# We specify an empty command so that we can pass options to the ENTRYPOINT command.
# This is a bit of a Dockerfile quirk where if the ENTRYPOINT value is defined,
# then CMD becomes the default options passed to ENTRYPOINT.
# In this case we don't have any desired default arguments.
# However, we have to specify CMD to enable passing of command line parameters to ENTRYPOINT in the first place.
CMD [  ]
