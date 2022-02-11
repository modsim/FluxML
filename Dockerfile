FROM quay.io/pypa/manylinux2014_x86_64
MAINTAINER Richard D. Paul r.paul@fz-juelich.de

RUN yum -y install gmp-devel 
RUN yum -y install xerces-c-devel 
RUN yum -y install flex 
RUN yum -y install bison 
RUN yum -y install re2c 
RUN yum -y install boost169-devel 
RUN yum -y install lapack-devel 
RUN yum -y install blas-devel
RUN yum -y install zlib-devel

RUN LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64

ADD . /root/FluxML
WORKDIR /root/FluxML
RUN ./config/mkautotools.sh boot
RUN ./configure --prefix=/root/fluxml_build 
RUN CPATH=/usr/include/boost169/ make
RUN make install

