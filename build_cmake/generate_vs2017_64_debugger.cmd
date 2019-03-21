@echo off
mkdir build_debugger
pushd build_debugger
cmake ../.. -G "Visual Studio 15 2017 Win64" -T v141_xp -DUSE_QT=on -DBUILD_PSFPLAYER=on -DDEBUGGER_INCLUDED=on -DCMAKE_PREFIX_PATH="C:\Qt\5.12.1\msvc2017_64"
popd
