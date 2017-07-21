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

RUN apt-get -y install mosquitto



