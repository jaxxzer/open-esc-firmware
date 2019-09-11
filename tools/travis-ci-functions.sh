#!/usr/bin/env bash

# print control for echob()
bold=$(tput bold)
normal=$(tput sgr0)

# counter for travis_fold: https://github.com/travis-ci/travis-ci/issues/2285
if [ -z $testN ]; then testN=0; fi

# echo with bold text
echob() {
    echo "${bold}${@}${normal}"
}

# run command helper for ci scripts
test() {
    echo -en "travis_fold:start:$testN\r                                  \r"
    echob "$@"
    time "$@"
    exitcode=$?
    echo -en "travis_fold:end:$testN\r                                    \r"
    echob "$@ exited with $exitcode"
    if [ $exitcode -ne 0 ]; then exit $exitcode; fi
    testN=$(($testN+1))
}
