#!/bin/sh

DIR=`pwd`
echo "building $DIR"
cd ../../ParticleFirmware/main
make all PLATFORM=photon APPDIR=$DIR DEBUG_BUILD=y -s
