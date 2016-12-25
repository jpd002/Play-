#/bin/bash

travis_before_install() 
{
    sudo add-apt-repository --yes ppa:beineri/opt-qt562-trusty
    sudo apt-get update -qq
    sudo apt-get install -qq qt56base
    cd ..
    git clone -q https://github.com/jpd002/Play-Build.git Play-Build
    pushd Play-Build
    git submodule update -q --init --recursive
    git submodule foreach "git checkout -q master"
    rm -rf Play
    mv ../Play- Play
    popd
}

travis_script()
{
    if [ "$CXX" = "g++" ]; then export CXX="g++-5" CC="gcc-5"; fi
    source /opt/qt56/bin/qt56-env.sh || true
    cd build_unix
    ./build.sh
}

set -e
set -x

$1;
