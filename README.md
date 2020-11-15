# VFD for Linux Tv Box

<p align=center>
    <img src="https://discourse.coreelec.org/uploads/default/original/1X/c19ba0116b4a988ae72e6284bb738aa3a73862d6.jpg" width=250>
  <br>
  <br>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg">
<img src="https://img.shields.io/badge/License-Apache%202.0-blue.svg">
  <img src="https://img.shields.io/badge/Vfd-Linux-brightgreen">
    <img src="https://img.shields.io/badge/Tanix-Tx3-red">
</p>

> 👩🏽‍💻 Full control over vfd
## What is this ?
This is a c program which interacts directly with the linux kernal. It gives you full access to control the lcd of your linux tv box.

## Installation
``` 
git clone https://github.com/micaelillos/vfdd.git/
cd vfdd
make # to compile
make install # to install
```

## Usage 
I have already compiled a generic version that can be used.
First make sure that the vfd driver is inserted into the kernal if not run:
```
insmod /lib/modules/$(uname -r)/extra/vfdmod.ko
```
### Running the Program
The program gets three parameters
```
./vfdd <WORD> <COLON> <NET>
```
> WORD - The word you want to display ex: "HEY"

> COLON - Should display Colon 1 for yes 0 for no

> NET - Should display Net 1 for yes 0 for no

#### Examples
```
./vfdd HEY 0 0
```