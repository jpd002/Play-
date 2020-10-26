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

            wget -q -O vulkansdk.tar.gz https://vulkan.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.gz?Human=true
            tar -zxf vulkansdk.tar.gz

            sudo add-apt-repository --yes ppa:beineri/opt-qt-5.12.3-xenial
            sudo apt update -qq
            sudo apt install -qq qt512base qt512x11extras gcc-9 g++-9 libgl1-mesa-dev libglu1-mesa-dev libalut-dev libevdev-dev
        fi
    elif [ "$TARGET_OS" = "Linux_Clang_Format" ]; then
        wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
        sudo apt-add-repository --yes "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-6.0 main"
        sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
        sudo apt-get update -y
        sudo apt-get install -y clang-format-6.0
    elif [ "$TARGET_OS" = "OSX" ]; then
        npm install -g appdmg
        curl -L --show-error --output vulkansdk.tar.gz https://vulkan.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/mac/vulkansdk-macos-${VULKAN_SDK_VERSION}.tar.gz?Human=true
        tar -zxf vulkansdk.tar.gz
    elif [ "$TARGET_OS" = "IOS" ]; then
        brew update
        brew install dpkg
    elif [ "$TARGET_OS" = "Android" ]; then
        echo y | sdkmanager 'ndk;21.3.6528147'
        echo y | sdkmanager 'cmake;3.10.2.4988404'
    elif [ "$TARGET_OS" = "FREEBSD" ]; then
        su -m root -c 'pkg install -y cmake qt5 evdev-proto'
    fi;

    git fetch --tags
    git submodule update -q --init --recursive
}

travis_script()
{
    if [ "$TARGET_OS" = "Android" ]; then
        if [ "$BUILD_LIBRETRO" = "yes" ]; then
            CMAKE_PATH=/usr/local/android-sdk/cmake/3.10.2.4988404
            export PATH=${CMAKE_PATH}/bin:$PATH
            export NINJA_EXE=${CMAKE_PATH}/bin/ninja
            export ANDROID_NDK=/usr/local/android-sdk/ndk/21.3.6528147
            export ANDROID_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
            pushd build_retro
            bash android_build.sh
            popd
        else
            pushd build_android
            ./gradlew
            ./gradlew assembleRelease
            popd
        fi
    elif [ "$TARGET_OS" = "Linux_Clang_Format" ]; then
        set +e
        find ./Source/ ./tools/ -iname *.h -o -iname *.cpp -o -iname *.m  -iname *.mm | xargs clang-format-6.0 -i
        git config --global user.name "Clang-Format"
        git config --global user.email "Clang-Format"
        git commit -am"Clang-format";
        if [ $? -eq 0 ]; then
            url=$(git format-patch -1 HEAD --stdout | nc termbin.com 9999)
            echo "generated clang-format patch can be found at: $url"
            echo "you can pipe patch directly using the following command:";
            echo "curl $url | git apply -v"
            echo "then manually commit and push the changes"
            exit -1;
        fi
        exit 0;
    else
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
        elif [ "$TARGET_OS" = "OSX" ]; then
            export CMAKE_PREFIX_PATH="$(brew --prefix qt5)"
            export VULKAN_SDK=$(pwd)/../vulkansdk-macos-${VULKAN_SDK_VERSION}/macOS
            cmake .. -G"$BUILD_TYPE" -DBUILD_LIBRETRO_CORE=yes
            cmake --build . --config Release
            ctest -C Release
            $(brew --prefix qt5)/bin/macdeployqt Source/ui_qt/Release/Play.app
            if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then
                ../.travis.macos.import_certificate.sh
                ../installer_macos/sign.sh
            fi;
            appdmg ../installer_macos/spec.json Play.dmg
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
    fi;
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
    if [ "$TARGET_OS" = "Android" ]; then
        if [ "$BUILD_LIBRETRO" = "yes" ]; then
            cp ../../build_retro/play_* .
            ABI_LIST="arm64-v8a armeabi-v7a x86 x86_64"
        else
            cp ../../build_android/build/outputs/apk/release/Play-release-unsigned.apk .
            cp Play-release-unsigned.apk Play-release.apk
            export ANDROID_BUILD_TOOLS=$ANDROID_HOME/build-tools/29.0.3
            $ANDROID_BUILD_TOOLS/apksigner sign --ks ../../installer_android/deploy.keystore --ks-key-alias deploy --ks-pass env:ANDROID_KEYSTORE_PASS --key-pass env:ANDROID_KEYSTORE_PASS Play-release.apk
            $ANDROID_BUILD_TOOLS/zipalign -c -v 4 Play-release.apk
            $ANDROID_BUILD_TOOLS/zipalign -c -v 4 Play-release-unsigned.apk
        fi
    fi;
    if [ "$TARGET_OS" = "OSX" ]; then
        ../../.travis.macos.notarize.sh
        cp ../../build/Play.dmg .
        cp ../../build/Source/ui_libretro/Release/play_libretro.dylib play_libretro_macOS-x86_64.dylib
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
