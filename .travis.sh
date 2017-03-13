#/bin/bash

travis_before_install() 
{
    cd ..
    if [ "$TARGET_OS" = "Linux" ]; then
        sudo add-apt-repository --yes ppa:beineri/opt-qt562-trusty;
        sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test;
        sudo apt-get update -qq;
        sudo apt-get install -qq qt56base gcc-5 g++-5 cmake libalut-dev;
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
        cd build_android
        chmod 755 gradlew
        ./gradlew
        ./gradlew assembleRelease
    else
        cd build_cmake
        mkdir build
        cd build

        if [ "$TARGET_OS" = "Linux" ]; then
            if [ "$CXX" = "g++" ]; then export CXX="g++-5" CC="gcc-5"; fi
            source /opt/qt56/bin/qt56-env.sh || true
            cmake .. -G"$BUILD_TYPE" -DCMAKE_PREFIX_PATH=/opt/qt56/;
            cmake --build .
        elif [ "$TARGET_OS" = "OSX" ]; then
            cmake .. -G"$BUILD_TYPE"
            cmake --build . --config Release
        elif [ "$TARGET_OS" = "IOS" ]; then
            cmake .. -G"$BUILD_TYPE" -DCMAKE_TOOLCHAIN_FILE=../../../Dependencies/cmake-ios/ios.cmake -DTARGET_IOS=ON
            cmake --build . --config Release
        fi;
    fi;
}

set -e
set -x

$1;
