# Dockerfile for building DO client components for Debian 10 amd64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Alternatively, use the command line:
# Copy the build script to the build directory:
# cp <src root>/build/scripts/bootstrap.sh <src root>/build/docker/debian10/amd64
#
# After running the above, you can build the image by running in the current dockerfile directory:
# sudo docker build -t debian10_amd64 . --no-cache --network=host
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# sudo docker run -ti --rm --entrypoint=/bin/bash -v ~/code/do-client/:/code debian10_amd64

FROM mcr.microsoft.com/mirror/docker/library/debian:buster@sha256:0e328f15f5cb0f93bff52a37d98f0bbfc40f97fcaf250096fd2b2c6871a497eb

SHELL [ "/bin/bash", "-c"]

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
