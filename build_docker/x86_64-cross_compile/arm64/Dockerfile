FROM debian:stretch-slim

WORKDIR /workdir

ENV CROSS_TRIPLE aarch64-none-linux-gnu
ENV QT_SELECT qt5

RUN dpkg --add-architecture arm64 && apt-get update
RUN apt-get install -y xz-utils make

ADD https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz /tmp/
RUN unxz < /tmp/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz > /tmp/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar
RUN tar xf /tmp/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar -C /tmp/
RUN cp -R /tmp/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/* /usr/
RUN ln -s /usr/aarch64-none-linux-gnu/libc/lib64/ /usr/lib/aarch64-linux-gnu

ADD https://github.com/Kitware/CMake/releases/download/v3.20.0-rc1/cmake-3.20.0-rc1-Linux-x86_64.sh /tmp/
RUN sh /tmp/cmake-3.20.0-rc1-Linux-x86_64.sh -- --skip-license --prefix=/usr/local

RUN apt-get install -y libevdev-dev:arm64 qtbase5-dev:arm64 libcurl4-openssl-dev:arm64 libssl-dev:arm64 libgl1-mesa-dev:arm64 libglu1-mesa-dev:arm64 libalut-dev:arm64 libqt5x11extras5-dev:arm64 libx11-dev:arm64
