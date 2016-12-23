#/bin/bash

travis_before_install() {
    git submodule update --init --recursive
    echo "Test!"
}

set -e
set -x

$1;
