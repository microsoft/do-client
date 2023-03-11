# Dockerfile for building DO apt plugin for linux-arm (64bit).
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-ubuntu1804-arm64

FROM mcr.microsoft.com/mirror/docker/library/ubuntu:18.04@sha256:6a4619c02fbaf80504f316f42bd4b732831d9590e9c1c0df2b6f294ffbee86c9

SHELL [ "/bin/bash", "-c"]

# QEMU is a Linux emulator which enables cross-arch support in docker
# In order to build this image on a Linux host, need to install QEMU:
#
# sudo apt-get install qemu-user
# update-binfmts --display
# sudo apt install qemu binfmt-support qemu-user-static
# cp /usr/bin/qemu-aarch64-static <src root>/build/docker/ubuntu1804/arm64
#
# Then copy the build script to the build directory
# cp <src root>/build/scripts/bootstrap.sh <src root>build/docker/ubuntu1804/arm64
#
# After running the above, you can build the image by running in the current dockerfile directory
# sudo docker build -t <your image name> . --no-cache --network=host

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
