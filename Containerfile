ARG DEBIAN_VERSION=stable-slim

FROM docker.io/library/debian:${DEBIAN_VERSION}

ARG QT_ABI=gcc_64
ARG QT_ARCH=
ARG QT_HOST=linux
ARG QT_TARGET=desktop
ARG QT_VERSION=6.10.2

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get --assume-yes upgrade \
  && apt-get --assume-yes autopurge

# Use Adoptium for Java 17
RUN apt-get --assume-yes --no-install-recommends install \
  apt-transport-https ca-certificates curl gpg
RUN curl -L https://packages.adoptium.net/artifactory/api/gpg/key/public | gpg --dearmor | tee /etc/apt/trusted.gpg.d/adoptium.gpg
RUN echo "deb https://packages.adoptium.net/artifactory/deb $(awk -F= '/^VERSION_CODENAME/{print$2}' /etc/os-release) main" | tee /etc/apt/sources.list.d/adoptium.list
RUN apt-get update

# Install base dependencies
RUN apt-get --assume-yes --no-install-recommends install \
  # Compilers
  clang lld llvm temurin-17-jdk \
  # Build system
  cmake ninja-build extra-cmake-modules pkg-config \
  # Dependencies
  cmark gamemode-dev libarchive-dev libcmark-dev libgamemode0 libgl1-mesa-dev libqrencode-dev libtomlplusplus-dev scdoc zlib1g-dev \
  # Tooling
  clang-format clang-tidy git

# Use LLD by default for faster linking
ENV CMAKE_LINKER_TYPE=lld

# Prepare and install Qt
## Setup UTF-8 locale (required, apparently)
RUN apt-get --assume-yes --no-install-recommends install locales
RUN echo "C.UTF-8 UTF-8" > /etc/locale.gen
RUN locale-gen
ENV LC_ALL=C.UTF-8

## Some libraries are required for the official binaries
RUN apt-get --assume-yes --no-install-recommends install \
  libglib2.0-0t64 libxkbcommon0 python3-pip

RUN pip3 install --break-system-packages aqtinstall
RUN aqt install-qt \
  ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_ARCH} \
  --outputdir /opt/qt \
  --modules qtimageformats qtnetworkauth

ENV PATH=/opt/qt/${QT_VERSION}/${QT_ABI}/bin:$PATH
ENV QT_PLUGIN_PATH=/opt/qt/${QT_VERSION}/${QT_ABI}/plugins/

## We don't use these. Nuke them
RUN rm -rf \
  "$QT_PLUGIN_PATH"/designer \
  "$QT_PLUGIN_PATH"/help \
  # "$QT_PLUGIN_PATH"/platformthemes/libqgtk3.so \
  "$QT_PLUGIN_PATH"/printsupport \
  "$QT_PLUGIN_PATH"/qmllint \
  "$QT_PLUGIN_PATH"/qmlls \
  "$QT_PLUGIN_PATH"/qmltooling \
  "$QT_PLUGIN_PATH"/sqldrivers

# Setup workspace
RUN mkdir /work
WORKDIR /work

ENTRYPOINT ["bash"]
CMD ["-i"]
