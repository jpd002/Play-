#/bin/bash

travis_before_install() 
{
    sudo add-apt-repository --yes ppa:beineri/opt-qt57-trusty
    sudo apt-get update
    sudo apt-get install qt5-qmake qtbase5-dev
    cd ..
    git clone https://github.com/jpd002/Play-Build.git Play-Build
    pushd Play-Build
    git submodule update --init --recursive
    git submodule foreach "git checkout master"
    rm -rf Play
    mv ../Play- Play
    popd
}

travis_script()
{
   cd build_unix
   ./build.sh
}

set -e
set -x

$1;
