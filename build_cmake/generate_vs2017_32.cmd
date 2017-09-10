@echo off
mkdir build
pushd build
cmake .. -G "Visual Studio 15 2017" -T v141_xp
popd
