#!/usr/bin/env bash

source tools/travis-ci-functions.sh

test tools/generate-nvic-all.sh

test export PATH=$(pwd)/arm8/gcc-arm-none-eabi-8-2019-q3-update/bin:$PATH

mkdir -p build
pushd build

# we are excluding gsc until we add GD32F3 support to libopencm3
for TARGET_BOARD in $(ls ../src/target -I inc -I gsc -I target.cmake)
do
    echob "selecting TARGET_BOARD: ${TARGET_BOARD}"
    test cmake --configure -DTARGET_BOARD=${TARGET_BOARD} ..
    test make
done
popd
