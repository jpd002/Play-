#/bin/bash

travis_before_install() 
{
    sudo apt-get update -qq
    sudo apt-get install cmake libboost-system-dev libboost-chrono-dev
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
