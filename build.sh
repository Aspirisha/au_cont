#!/bin/bash

rm -rf build
mkdir -p build
cd build
cmake -G "Unix Makefiles" ../src
make 
cp __aucont_start ../bin
cp __aucont_exec ../bin
cp ../src/aucont_list ../bin
cp ../src/aucont_stop ../bin
cp ../src/aucont_exec ../bin
cp ../src/aucont_start ../bin
cp ../src/__setup_network.sh ../bin
cp ../src/aucontd ../bin
cd ../bin

#
# this is only to make tests pass as they are
# IRL it shouldn't be called from build script
echo "/=========================================================================\\"
echo "| Now you'll be prompted to enter your sudo password. This is done        |"
echo "| intentionally, the daemon for aucont needs to run with sudo privileges. |"
echo "\=========================================================================/"
sudo twistd -y ../bin/aucontd