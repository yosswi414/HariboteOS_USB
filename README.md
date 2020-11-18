# HariboteOS_USB
HariboteOS that can boot from USB or something.

## How to build
```
$ make
```

## How to boot on VirtualBox
```
$ make run
```
(You need to set up virtual machine for the first time. This part will be automated in future.)

## How to boot on real machine
Burn the output file `usbboot.img` to the target stick memory or some sort and it will boot.

## Prerequisites
You need to install below using `sudo apt install`.
- VirtualBox (`vboxmanage`)
- `nasm`
- `mtools` ( `mformat`, `mcopy` )
- `libc6-i386`
- z_tools ( `z_tools/makefont.exe` )

## Note
- The command `exit` works only in VirtualBox environment.
