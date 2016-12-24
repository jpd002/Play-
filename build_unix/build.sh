set -e
mkdir -p build
cd build
cmake ..
make
cd ..
mkdir -p build-ui/
qmake -o build-ui/
cd build-ui
make
cd ..
