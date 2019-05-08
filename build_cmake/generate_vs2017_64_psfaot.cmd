@echo off
mkdir build_psfaot
pushd build_psfaot
cmake ../.. -G "Visual Studio 15 2017 Win64" -T v141_xp -DBUILD_PSFPLAYER=on -DBUILD_PLAY=off -DBUILD_TESTS=off -DBUILD_AOT_CACHE=on
popd
