#/bin/bash

travis_before_install() 
{
    cd ..
    if [ "$TARGET_OS" = "Linux" ]; then
        sudo add-apt-repository --yes ppa:beineri/opt-qt591-trusty
        sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
        sudo apt-get update -qq
        sudo apt-get install -qq qt59base gcc-5 g++-5 libalut-dev
        curl -sSL https://cmake.org/files/v3.8/cmake-3.8.1-Linux-x86_64.tar.gz | sudo tar -xzC /opt
    elif [ "$TARGET_OS" = "OSX" ]; then
        sudo npm install -g appdmg
    elif [ "$TARGET_OS" = "IOS" ]; then
        brew update
        brew install dpkg
    elif [ "$TARGET_OS" = "Android" ]; then
        sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y #is this needed?
        sudo apt-get update -y
        sudo apt-get install libstdc++6 -y

        wget http://dl.google.com/android/repository/android-ndk-r14-linux-x86_64.zip
        unzip android-ndk-r14-linux-x86_64.zip>/dev/null
        export ANDROID_NDK=$(pwd)/android-ndk-r14
        echo "ndk.dir=$ANDROID_NDK">./Play-/build_android/local.properties

        wget https://github.com/Commit451/android-cmake-installer/releases/download/1.1.0/install-cmake.sh
        chmod +x install-cmake.sh>/dev/null
        ./install-cmake.sh
    fi;

    git clone -q https://github.com/jpd002/Play-Build.git Play-Build
    pushd Play-Build
    git submodule update -q --init --recursive
    git submodule foreach "git checkout -q master"
    cd Dependencies
    git submodule update --init
    cd ..
    rm -rf Play
    mv ../Play- Play
    popd
}

travis_script()
{
    if [ "$TARGET_OS" = "Android" ]; then
        pushd build_android
        ./gradlew
        ./gradlew assembleRelease
        popd 
    else
        pushd build_cmake
        
        mkdir build
        pushd build
        
        if [ "$TARGET_OS" = "Linux" ]; then
            if [ "$CXX" = "g++" ]; then export CXX="g++-5" CC="gcc-5"; fi
            export PATH=/opt/cmake-3.8.1-Linux-x86_64/bin/:$PATH
            source /opt/qt59/bin/qt59-env.sh || true
            cmake .. -G"$BUILD_TYPE" -DCMAKE_PREFIX_PATH=/opt/qt591/ -DCMAKE_INSTALL_PREFIX=./appdir/usr;
            cmake --build .
            cmake --build . --target install
            # AppImage Creation
            wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
            chmod a+x linuxdeployqt*.AppImage
            unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
            ./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs
            ./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -appimage
        elif [ "$TARGET_OS" = "OSX" ]; then
            cmake .. -G"$BUILD_TYPE"
            cmake --build . --config Release
            appdmg ../../installer_macosx/spec.json Play.dmg
        elif [ "$TARGET_OS" = "IOS" ]; then
            cmake .. -G"$BUILD_TYPE" -DCMAKE_TOOLCHAIN_FILE=../../../Dependencies/cmake-ios/ios.cmake -DTARGET_IOS=ON
            cmake --build . --config Release
            codesign -s "-" Release-iphoneos/Play.app
            pushd ..
            pushd ..
            pushd installer_ios
            ./build.sh
            popd
            popd
            popd
        fi;
        
        popd
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
    if [ -z "$ANDROID_KEYSTORE_PASS" ]; then
        return
    fi;
    if [ "$TARGET_OS" = "Android" ]; then
        cp ../../build_android/build/outputs/apk/Play-release-unsigned.apk .
        export ANDROID_BUILD_TOOLS=$ANDROID_HOME/build-tools/25.0.0
        $ANDROID_BUILD_TOOLS/zipalign -v -p 4 Play-release-unsigned.apk Play-release.apk
        $ANDROID_BUILD_TOOLS/apksigner sign --ks ../../installer_android/deploy.keystore --ks-key-alias deploy --ks-pass env:ANDROID_KEYSTORE_PASS --key-pass env:ANDROID_KEYSTORE_PASS Play-release.apk
    fi;
    if [ "$TARGET_OS" = "Linux" ]; then
        cp ../../build_cmake/build/Play.AppImage .
    fi;
    if [ "$TARGET_OS" = "OSX" ]; then
        cp ../../build_cmake/build/Play.dmg .
    fi;
    if [ "$TARGET_OS" = "IOS" ]; then
        cp ../../installer_ios/Play.deb .
        cp ../../installer_ios/Packages.bz2 .
    fi;
    popd
    popd
}

set -e
set -x

$1;
