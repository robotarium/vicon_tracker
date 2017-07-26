FROM ubuntu:16.04

RUN apt-get -y update
RUN apt-get -y upgrade

RUN apt-get -y install git

RUN mkdir /home/programs
ENV PROGRAMDIR /home/programs

RUN apt-get -y install g++ cmake

RUN cd $PROGRAMDIR && \
git clone https://github.com/vrpn/vrpn.git && \
cd vrpn && mkdir build && cd build && \
cmake .. && make -j $(($(nproc)-1)) && make install

# Get mosquitto prereqs
RUN apt-get -y install libc-ares-dev uuid-dev xsltproc

RUN apt-get -y install wget

# Install Mosquitto
RUN cd $PROGRAMDIR && \
    wget http://mosquitto.org/files/source/mosquitto-1.4.10.tar.gz && \
    tar -xvzf mosquitto-1.4.10.tar.gz && \
    cd mosquitto-1.4.10 && \
    make -j $(($(nproc) - 1)) && \
    make install

RUN cd $PROGRAMDIR && \
        git clone https://github.com/nlohmann/json.git && \
        cd json && \
        mkdir build && \
        cd build/ && \
        cmake .. && \
        make -j $(($(nproc) - 1)) && \
        make install && \
    ln -s /usr/local/include/nlohmann/json.hpp /usr/include/json.hpp

RUN cd $PROGRAMDIR && \
git clone https://github.com/robotarium/vicon_tracker.git \
&& cd vicon_tracker && mkdir build && cd build && \
cmake .. && make

RUN apt-get -y install python3 python3-pip

# Get vizier framework
RUN git clone https://github.com/robotarium/vizier.git
RUN pip3 install -r vizier/python/requirements.txt vizier/python/

