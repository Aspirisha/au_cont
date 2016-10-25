#!/bin/bash

rm -rf build
mkdir -p build
cd build
cmake -G "Unix Makefiles" ../src
make 
cp ../src/aucont_list ../bin
cp ../src/aucont_stop ../bin
cp ../src/aucont_exec ../bin
cp ../src/aucontd ../bin
cd ../bin
twistd -y aucontd