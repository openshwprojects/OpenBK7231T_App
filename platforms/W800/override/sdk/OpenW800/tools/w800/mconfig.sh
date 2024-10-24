#!/bin/sh

cd tools/w800/config
make mconf
cd ..
../../bin/build/config/mconf wconfig