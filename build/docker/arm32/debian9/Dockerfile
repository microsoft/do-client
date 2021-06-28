# Dockerfile for building DO apt plugin for linux-arm (32bit).
# First, install the docker extension for VSCode. Then you can right-click on this file
# and choose Build Image. Give it a name and it will build the image.
#
# Open interactive terminal into the image in a container:
# docker run -ti --rm --entrypoint=/bin/bash -v <project root dir>:/code -v <build root dir>:/build <image_name>
# Example:
# docker run -ti --rm --entrypoint=/bin/bash -v D:\do-client-lite:/code -v D:\temp\build_client_lite\arm-linux-debug:/build custom-debian9-arm32

FROM arm32v7/debian:stretch

SHELL [ "/bin/bash", "-c"]

# QEMU is a Linux emulator which enables cross-arch support in docker
# In order to build this image on a Linux host, need to install QEMU:
#
# sudo apt-get install qemu-user
# update-binfmts --display
# sudo apt install qemu binfmt-support qemu-user-static
# cp /usr/bin/qemu-arm-static <src root>/build/docker/arm32/debian9
#
# Then copy the build script to the build directory
# cp <src root>/build/bootstrap/bootstrap-debian-9.sh <src root>build/docker/arm32/debian9
#
# After running the above, you can build the image by running in the current dockerfile directory
# sudo docker build -t <your image name> . --no-cache --network=host

COPY qemu-arm-static /usr/bin/qemu-arm-static
COPY bootstrap-debian-9.sh /tmp/bootstrap-debian-9.sh

WORKDIR /tmp/
RUN chmod +x bootstrap-debian-9.sh
RUN ./bootstrap-debian-9.sh


VOLUME /code
WORKDIR /code

# NOTE: For azure pipelines, it is required that entrypoint is NOT defined
# See: https://docs.microsoft.com/en-us/azure/devops/pipelines/process/container-phases?view=azure-devops&tabs=yaml
# When building docker images locally, and using them for local builds, uncomment the below line 
#ENTRYPOINT [ "/bin/bash", "-c"]

# We specify an empty command so that we can pass options to the ENTRYPOINT command.
# This is a bit of a Dockerfile quirk where if the ENTRYPOINT value is defined,
# then CMD becomes the default options passed to ENTRYPOINT.
# In this case we don't have any desired default arguments.
# However, we have to specify CMD to enable passing of command line parameters to ENTRYPOINT in the first place.
CMD [  ]
