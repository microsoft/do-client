# Dockerfile for building DO client components for Debian 12 amd64
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Alternatively, use the command line:
# Copy the build script to the build directory:
# cp <src root>/build/scripts/bootstrap.sh <src root>/build/docker/debian12/amd64
#
# After running the above, you can build the image by running in the current dockerfile directory:
# sudo docker build -t debian12_amd64 . --no-cache --network=host
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# sudo docker run -ti --rm --entrypoint=/bin/bash -v ~/code/do-client/:/code debian12_amd64

# Find the sha256 digest using:
# sudo docker manifest inspect mcr.microsoft.com/mirror/docker/library/debian:bookworm

FROM mcr.microsoft.com/mirror/docker/library/debian:bookworm@sha256:9d8be01b7374ef07c68468882782c1226c532f39145f46ad969b6fbd414ab730

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
