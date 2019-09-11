#!/usr/bin/env bash

source tools/travis-ci-functions.sh

test tools/install-arm-toolchain.sh
test tools/build-all.sh
