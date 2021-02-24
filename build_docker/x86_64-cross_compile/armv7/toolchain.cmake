set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER "arm-none-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-none-linux-gnueabihf-g++")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-rpath-link,/usr/lib/arm-linux-gnueabihf/ -Wl,-rpath-link,/lib/arm-linux-gnueabihf/ -Wl,-rpath-link,/usr/arm-none-linux-gnueabihf/lib/")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I /usr/include/arm-linux-gnueabihf/ -I /usr/include/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /usr/include/arm-linux-gnueabihf/ -I /usr/include/")

set(CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/arm-linux-gnueabihf/;/lib/arm-linux-gnueabihf/;/usr/arm-none-linux-gnueabihf/lib/;")
set(CMAKE_SYSTEM_INCLUDE_PATH "${CMAKE_SYSTEM_INCLUDE_PATH};/usr/include/;/usr/include/arm-linux-gnueabihf/;")
