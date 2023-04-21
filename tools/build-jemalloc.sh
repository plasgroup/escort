#!/bin/bash

CONFIG="--with-jemalloc-prefix=_escort_internal_ --disable-cxx --enable-autogen"
CONFIG_RELEASE="$CONFIG --with-install-suffix=escort"
CONFIG_DEBUG="$CONFIG --with-install-suffix=escortdbg --enable-debug"

cd jemalloc

./configure $CONFIG_RELEASE
make clean
make -j

./configure $CONFIG_DEBUG
make clean
make -j

