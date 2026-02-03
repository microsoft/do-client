# Dockerfile for building DO client components for Debian 12 arm64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-debian12-arm64

# Find the sha256 digest using:
# sudo docker manifest inspect mcr.microsoft.com/mirror/docker/library/debian:bookworm

FROM mcr.microsoft.com/mirror/docker/library/debian:bookworm@sha256:629bbf0466a7110f5928480d1579cacc09bf47e91c4e0f9d4558b46be140cb9f

SHELL [ "/bin/bash", "-c"]

# QEMU is required for cross-arch support when building on amd64/x64 hosts.
# On Windows with Docker Desktop (WSL2), run this once to enable QEMU:
#   docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
#
# On Linux, install QEMU:
#   sudo apt install qemu binfmt-support qemu-user-static
#   docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
#
# Then copy the build script to the build directory:
#   cp <src root>/build/scripts/bootstrap.sh <src root>/build/docker/debian12/arm64
#
# Build the image:
#   sudo docker build -t debian12_arm64 . --no-cache --network=host

COPY qemu-arm-static /usr/bin/qemu-arm-static
COPY bootstrap.sh /tmp/bootstrap.sh

WORKDIR /tmp/
RUN chmod +x bootstrap.sh
RUN ./bootstrap.sh --install build

VOLUME /code
WORKDIR /code

ENTRYPOINT [ "/bin/bash", "-c" ]

# We specify an empty command so that we can pass options to the ENTRYPOINT command.
# This is a bit of a Dockerfile quirk where if the ENTRYPOINT value is defined,
# then CMD becomes the default options passed to ENTRYPOINT.
# In this case we don't have any desired default arguments.
# However, we have to specify CMD to enable passing of command line parameters to ENTRYPOINT in the first place.
CMD [  ]
