#!/bin/bash
cd ../lib/qBreakpad

echo "Updating qBreakpad's breakpad dependency version"
cd third_party/breakpad
git checkout 7b3afa9258e58a57ffbeb395d445811f92616ae9
cd ../../

cd handler
mkdir build
cd build
qmake ..
echo "Building handler"
make -j8 && "Successfully built qBreakpad"
