set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER "aarch64-none-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-none-linux-gnu-g++")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-rpath-link,/usr/lib/aarch64-linux-gnu/  -Wl,-rpath-link,/lib/aarch64-linux-gnu/ -Wl,-rpath-link,/usr/aarch64-linux-gnu/lib/")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I /usr/include/aarch64-linux-gnu/ -I /usr/include/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /usr/include/aarch64-linux-gnu/ -I /usr/include/")

set(CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/aarch64-linux-gnu/;/lib/aarch64-linux-gnu/;/usr/aarch64-linux-gnu/lib/;")
set(CMAKE_SYSTEM_INCLUDE_PATH "${CMAKE_SYSTEM_INCLUDE_PATH};/usr/include/;/usr/include/aarch64-linux-gnu/;")

set(CMAKE_CROSSCOMPILING_EMULATOR /usr/bin/qemu-aarch64)
