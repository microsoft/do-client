# Dockerfile for building DO client components for Ubuntu 24.04 amd64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-ubuntu2404-amd64

# Find the sha256 digest using: 
# sudo docker manifest inspect mcr.microsoft.com/mirror/docker/library/ubuntu:24.04

FROM mcr.microsoft.com/mirror/docker/library/ubuntu:24.04@sha256:4fdf0125919d24aec972544669dcd7d6a26a8ad7e6561c73d5549bd6db258ac2

SHELL [ "/bin/bash", "-c"]

# You can build the image by running in the current dockerfile directory after copying the bootstrap.sh script:
# sudo docker build -t <your image name> . --no-cache --network=host

# Ubuntu 24.04 requires user prompt for apt-get update command, docker has issues handling this input
# ENV DEBIAN_FRONTEND=noninteractive

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
