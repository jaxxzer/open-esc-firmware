#!/usr/bin/env bash

source tools/travis-ci-functions.sh

[ ! -d arm9 ] || exit 0
mkdir -p arm9
pushd arm9
test wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
test tar -xf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
popd
test export PATH=$(pwd)/arm9/gcc-arm-none-eabi-9-2019-q4-major/bin:$PATH
