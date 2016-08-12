#!/bin/sh

DIR=`pwd`
echo "cleaning $DIR"
cd ../../ParticleFirmware/main
make clean PLATFORM=photon APPDIR=$DIR -s
