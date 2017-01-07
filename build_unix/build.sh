set -e
mkdir -p build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/Qt5.6.0/5.6/gcc_64/
make
