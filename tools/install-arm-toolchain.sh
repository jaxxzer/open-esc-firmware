#!/usr/bin/env bash

source tools/travis-ci-functions.sh

[ ! -d arm8 ] || exit 0
mkdir -p arm8
pushd arm8
test wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2
test tar -xf gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2
popd
test export PATH=$(pwd)/arm8/gcc-arm-none-eabi-8-2019-q3-update/bin:$PATH
