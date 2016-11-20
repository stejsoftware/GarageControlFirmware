### Building the Garage Controller Firmware ###

To build the Garage Controller firmware use [MinGW](http://www.mingw.org/) with the [Particle](https://docs.particle.io/guide/tools-and-features/cli/photon/) [node.js](https://nodejs.org/en/) app.

First login to the Particle servers

```shell
$ particle login
? Please enter your email address developer@server.com
? Please enter your password ******
> Successfully completed login!
```

Then to compile the firmware.

```shell
$ ./compile-release.sh
  building /c/GarageControl/GarageControlFirmware/GarageControlMaster
     text    data     bss     dec     hex filename
    25084     172    1848   27104    69e0 c:/GarageControl/GarageControlFirmware/GarageControlMaster/target/GarageControlMaster.elf
```

Next program via the Internet.

```shell
$ ./upload.sh
Including:
    target/GarageControlMaster.bin
attempting to flash firmware to your device stej_garage
Flash device OK:  Update started
```

Or program via USB port.

```shell
$ ./flash.sh
```
