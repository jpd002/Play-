FROM debian:stretch-slim

WORKDIR /workdir

ENV CROSS_TRIPLE arm-none-linux-gnueabihf
ENV QT_SELECT qt5

RUN dpkg --add-architecture armhf && apt-get update
RUN apt-get install -y xz-utils make

ADD https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz /tmp/
RUN unxz < /tmp/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz > /tmp/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar
RUN tar xf /tmp/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar -C /tmp/
RUN cp -R /tmp/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/* /usr/
RUN ln -s /usr/arm-none-linux-gnueabihf/libc/lib/ /usr/lib/arm-linux-gnueabihf

ADD https://github.com/Kitware/CMake/releases/download/v3.20.0-rc1/cmake-3.20.0-rc1-Linux-x86_64.sh /tmp/
RUN sh /tmp/cmake-3.20.0-rc1-Linux-x86_64.sh -- --skip-license --prefix=/usr/local

RUN apt-get install -y libevdev-dev:armhf qtbase5-dev:armhf libcurl4-openssl-dev:armhf libssl-dev:armhf libegl1-mesa-dev:armhf libgles2-mesa-dev:armhf libalut-dev:armhf libqt5x11extras5-dev:armhf libx11-dev:armhf
