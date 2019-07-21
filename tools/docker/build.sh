#!/bin/sh

if [ ! -d build-docker-release ]; then
    mkdir build-docker-release
fi

cd build-docker-release
qmake ..
make -j8
