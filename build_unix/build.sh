set -e
#mkdir -p build
#cd build
#cmake ..
#make
#cd ..
mkdir -p build-ui
cd build-ui
qmake ..
make
cd ..
