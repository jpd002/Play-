@echo off
mkdir build_vs2022
pushd build_vs2022
cmake ../.. -G "Visual Studio 17 2022" -A x64 -DUSE_QT=on -DBUILD_LIBRETRO_CORE=yes -DBUILD_PSFPLAYER=on -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
popd
