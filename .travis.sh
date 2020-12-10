#/bin/bash

travis_before_install() 
{
    if [ "$TARGET_OS" = "Linux" ]; then
        sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
        if [ "$TARGET_ARCH" = "ARM64" ]; then
            sudo apt update -qq
            sudo apt install -y gcc-9 g++-9 qtbase5-dev libqt5x11extras5-dev libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev libalut-dev libevdev-dev libgles2-mesa-dev
            wget https://purei.org/travis/cmake-3.17.1-Linux-ARM64.sh
            chmod 755 cmake-3.17.1-Linux-ARM64.sh
            sudo sh cmake-3.17.1-Linux-ARM64.sh --skip-license --prefix=/usr/local
        else
            wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
            chmod a+x linuxdeployqt*.AppImage

            wget -q -O vulkansdk.tar.gz https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.gz?Human=true
            tar -zxf vulkansdk.tar.gz

            sudo add-apt-repository --yes ppa:beineri/opt-qt-5.12.3-xenial
            sudo apt update -qq
            sudo apt install -qq qt512base qt512x11extras gcc-9 g++-9 libgl1-mesa-dev libglu1-mesa-dev libalut-dev libevdev-dev
        fi
    elif [ "$TARGET_OS" = "IOS" ]; then
        brew update
        brew install dpkg
    elif [ "$TARGET_OS" = "FREEBSD" ]; then
        su -m root -c 'pkg install -y cmake qt5 evdev-proto'
    fi;

    git fetch --tags
    git submodule update -q --init --recursive
}

travis_script()
{
    mkdir build
    pushd build
    
    if [ "$TARGET_OS" = "Linux" ]; then
        if [ "$CXX" = "g++" ]; then export CXX="g++-9" CC="gcc-9"; fi
        source /opt/qt512/bin/qt512-env.sh || true
        export VULKAN_SDK=$(pwd)/../${VULKAN_SDK_VERSION}/x86_64
        export PATH=$PATH:/opt/qt512/lib/cmake
        cmake .. -G"$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX=./appdir/usr -DBUILD_LIBRETRO_CORE=yes;
        cmake --build . -j $(nproc)
        ctest
        cmake --build . --target install
        if [ "$TARGET_ARCH" = "x86_64" ]; then
            mkdir -p appdir/usr/share/doc/libc6/
            echo "" > appdir/usr/share/doc/libc6/copyright
            # AppImage Creation
            unset QTDIR; unset QT_PLUGIN_PATH; unset LD_LIBRARY_PATH;
            export VERSION="${TRAVIS_COMMIT:0:8}"
            ../linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs -unsupported-allow-new-glibc -qmake=`which qmake`
            ../linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -appimage -unsupported-allow-new-glibc -qmake=`which qmake`
        fi
    elif [ "$TARGET_OS" = "IOS" ]; then
        cmake .. -G"$BUILD_TYPE" -DCMAKE_TOOLCHAIN_FILE=../deps/Dependencies/cmake-ios/ios.cmake -DTARGET_IOS=ON -DBUILD_PSFPLAYER=ON -DBUILD_LIBRETRO_CORE=yes
        cmake --build . --config Release
        codesign -s "-" Source/ui_ios/Release-iphoneos/Play.app
        pushd ..
        pushd installer_ios
        ./build_cydia.sh
        ./build_ipa.sh
        popd
        popd
    elif [ "$TARGET_OS" = "FREEBSD" ]; then
        export CXX="g++" CC="gcc"
        cmake ..
        cmake --build . -j$(sysctl -n hw.ncpu)
    fi;
    
    popd
}

travis_before_deploy()
{
    export SHORT_HASH="${TRAVIS_COMMIT:0:8}"
    mkdir deploy
    pushd deploy
    mkdir $SHORT_HASH
    pushd $SHORT_HASH
    if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
        return
    fi;
    if [ "$TARGET_OS" = "Linux" ]; then
        if [ "$TARGET_ARCH" = "x86_64" ]; then
            cp ../../build/Play*.AppImage .
            cp ../../build/Source/ui_libretro/play_libretro.so play_libretro_linux-x86_64.so
        else
            cp ../../build/Source/ui_libretro/play_libretro.so play_libretro_linux-ARM64.so
        fi;
    fi;
    if [ "$TARGET_OS" = "IOS" ]; then
        cp ../../installer_ios/Play.ipa .
        cp ../../installer_ios/Play.deb .
        cp ../../installer_ios/Packages.bz2 .
        cp ../../build/Source/ui_libretro/Release-iphoneos/play_libretro_ios.dylib play_libretro_ios.dylib
    fi;
    popd
    popd
}

set -e
set -x

$1;
