# Dockerfile for building DO client components for Ubuntu 20.04 arm64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-ubuntu2004-arm64

FROM mcr.microsoft.com/mirror/docker/library/ubuntu:20.04@sha256:c01a10535d9ea3e4065dee4abdb288821b2918929f1e1584287a03e731b9176c

SHELL [ "/bin/bash", "-c"]

# QEMU is a Linux emulator which enables cross-arch support in docker
# In order to build this image on a Linux host, need to install QEMU:
#
# sudo apt-get install qemu-user
# update-binfmts --display
# sudo apt install qemu binfmt-support qemu-user-static
# cp /usr/bin/qemu-aarch64-static <src root>/build/docker/ubuntu2004/arm64
#
# Then copy the build script to the build directory
# cp <src root>/build/bootstrap.sh <src root>build/docker/ubuntu2004/arm64
#
# After running the above, you can build the image by running in the current dockerfile directory
# sudo docker build -t <your image name> . --no-cache --network=host

# Ubuntu 20.04 requires user prompt for apt-get update command, docker has issues handling this input
ENV DEBIAN_FRONTEND=noninteractive

COPY qemu-aarch64-static /usr/bin/qemu-aarch64-static
COPY bootstrap.sh /tmp/bootstrap.sh

WORKDIR /tmp/
RUN chmod +x bootstrap.sh
RUN ./bootstrap.sh --install build

VOLUME /code
WORKDIR /code

ENTRYPOINT [ "/bin/bash", "-c"]

# We specify an empty command so that we can pass options to the ENTRYPOINT command.
# This is a bit of a Dockerfile quirk where if the ENTRYPOINT value is defined,
# then CMD becomes the default options passed to ENTRYPOINT.
# In this case we don't have any desired default arguments.
# However, we have to specify CMD to enable passing of command line parameters to ENTRYPOINT in the first place.
CMD [  ]
