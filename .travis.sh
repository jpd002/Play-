#/bin/bash

travis_before_install() {
    git clone https://github.com/jpd002/Play-Build.git Play-Build
    git submodule update --init --recursive
    git submodule foreach "git checkout master"
}

set -e
set -x

$1;
