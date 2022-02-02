FROM ubuntu:20.04
LABEL Maintainer="FluxML Dev Team <info@13cflux.net>"
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y
RUN apt-get upgrade -y
# general stuff
RUN apt-get install -y apt-utils
# FluxML dependencies
RUN apt-get install -y gfortran re2c libxerces-c-dev libblas-dev liblapack-dev libboost-dev libgmp-dev python3 python3-distutils zlib1g-dev
# install compiler and build-tools
RUN apt-get install -y build-essential gcc-9 automake autoconf libtool

ADD . /home/fluxml/src
WORKDIR /home/fluxml/src
RUN config/mkautotools.sh boot
RUN ./configure --libdir=/usr/lib/x86_64-linux-gnu
RUN make -j
RUN make install
WORKDIR /home/fluxml

