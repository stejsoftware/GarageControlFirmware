#!/bin/sh

DIR=`pwd`
echo "flashing $DIR"
cd ../../ParticleFirmware/main
make program-dfu PLATFORM=photon APPDIR=$DIR -s
