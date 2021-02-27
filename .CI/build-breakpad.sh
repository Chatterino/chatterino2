#!/bin/sh

set -e

mkdir lib/qBreakpad/handler/build || true
cd lib/qBreakpad/handler/build
# TODO: config = release or debug? same as CI!
qmake ..
make -j4
ls -l
