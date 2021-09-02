set EMSDK_LOCAL_ROOT=Z:\emsdk
set PATH=%PATH%;C:\Utils
call %EMSDK_LOCAL_ROOT%\emsdk_env.bat
mkdir build_wasm
pushd build_wasm
emcmake cmake ../.. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF -DBUILD_PLAY=OFF -DBUILD_PSFPLAYER=ON -DUSE_QT=OFF
