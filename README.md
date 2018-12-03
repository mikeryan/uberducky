# Uberducky - Wireless USB Rubber Ducky built on Ubertooth

Uberducky is a tool for injecting keystrokes into a target computer that can be
triggered wirelessly via BLE. It is inspired by Hak5's [USB Rubber
Ducky](https://shop.hak5.org/products/usb-rubber-ducky-deluxe), and like that
tool it impersonates a USB keyboard and can inject malicious payloads such as
reverse shells and RATs.

A normal USB Rubber Ducky has one major shortcoming: it is only useful when the
target machine is left unlocked. An Uberducky can be hidden in a forgotten USB
port, such as behind a tower under a desk or attached to a monitor. As soon as
the operator of the target system drops their guard (or you distract them),
Uberducky can be triggered to drop a malicious payload.

If you're interested you can [read the story behind
Uberducky](https://blog.ice9.us/2018/12/uberducky-ble-wireless-usb-rubber-ducky.html)
and learn more about how it's built.

# Getting Started

At the moment, you will need to build the firmware to use the tool and rebuild
it any time you want to change the duckyscript payload.

You will need `ubertooth-dfu` from [Project
Ubertooth](https://github.com/greatscottgadgets/ubertooth) to flash the
firmware. You will also need an ARM toolchain. If you are on Debian or Ubuntu,
install these with:

    apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi

This repo uses a submodule. If you didn't `git clone --recursive`, you must
pull down the submodule using:

    git submodule update --init --recursive

Drop your duckyscript payload into ```script.txt```. You can then build and
flash the firmware by running:

    make
    ubertooth-dfu -r -d uberducky.dfu

Big fat warning: this will replace any existing Ubertooth firmware you have on
the device. If you want to re-flash normal Ubertooth firmware, follow the
instructions below for how to re-flash.

## Triggering Uberducky

Uberducky will wait for a specially-formatted BLE advertising packet. When it
receives this packet, it will run the duckyscript. There are two ways of
triggering it. If you have a second Ubertooth, you can run the following:

    ubertooth-ducky -q

If you do not have a second Ubertooth but you have a system running Linux, you
can send the special packet using the following commands:

    sudo btmgmt add-adv -D 1 -u fd123ff9-9e30-45b2-af0d-b85b7d2dc80c 1 &&
        sudo btmgmt clr-adv

If neither of these approaches strikes your fancy, you may trigger Uberducky
using any mechanism that results in `fd123ff9-9e30-45b2-af0d-b85b7d2dc80c` being
in the first 32 bytes of any BLE advertising packet on channel 38 (2426 MHz) in
LE byte order (i.e., `0c c8 2d 7d...`). We're simply advertising it in a list of
128-bit UUIDs.

## Re-flashing the firmware

Since Uberducky impersonates a keyboard, it does not respond to normal USB
commands for reflashing the firmware. There are two mechanisms for forcing it to
re-enter DFU mode and allowing the firmware to be re-flashed. The first is to
send a specially-formatted BLE packet to reset it into DFU mode using a second
Ubertooth:

    ubertooth-ducky -b

It can also be done from Linux:

    sudo btmgmt add-adv -D 1 -u 344bc7f2-5619-4953-9be8-9888fe29d996 1 &&
        sudo btmgmt clr-adv

Alternatively, you can force the device into DFU mode by using a wire to jumper
two pins on the expansion header. 

The expansion header is a 2x3 pin header between the two largest chips
on the board, about halfway between the USB connector and antenna. It is
labeled "EXPAND" on the back side of the board. Holding the board so USB
is pointed up, on the front side of the board the expansion header looks
like this:

      +-------+
    6 + O | O | 3
      +---+---+
    5 + O | O | 2
      +---+---+
    4 + O | X | 1
      +---+---+

PIN #1, marked X, is square.

With Ubertooth unplugged, use a piece of wire, paperclip, staple, etc to
connect pins 1 and 2 together. When it it plugged in, the Ubertooth
should enter bootloader mode (LEDs will be turning on and off in
sequence).

You must flash the new firmware within 5 seconds, otherwise it will boot into
the previously flashed firmware.

# Future Work

I would like to implement some mechanism for updating the Duckyscript and
changing the UUID without having to recompile. Possible solutions include a USB
serial port and a wireless protocol.

It would be useful to have dedicated apps for triggering Uberducky on OS X,
Android, and possibly even iOS.

# Frequently Asked Questions

## Why Wireless?

Sometimes it's not feasible to wait for your target to leave their computer
unlocked. If their system has a hidden USB port (such as behind a screen or
under a desk) you can plug in Uberducky and lay in wait. When the opportunity
arises, distract the victim and launch the attack wirelessly.

## Why Ubertooth?

Ubertooth is a great piece of kit, and best of all the design is completely
open. The LPC1752 microcontroller has a flexible USB peripheral that can
emulate a keyboard, and the CC2400 radio facilitates easy communication over 2.4
GHz ISM. This tool was partly an exercise in seeing how little code it takes to
receive BLE advertising packets on Ubertooth (fewer than 100 lines of actual
code!).

## How do I change the script?

Update `script.txt` and rerun the `make` and `ubertooth-dfu` commands listed
above. We're working on better ways of doing this.

## I need Duckyscript payloads

[USB Rubber Ducky Payloads](https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Payloads).

## I want regular Ubertooth functionality back

See the above section on "re-flashing firmware".

## What do you think of Duckyscript?

No comment (hey there are kids around).

## I have an idea on how to make Uberducky even cooler!

Submit a GitHub issue with your feature request, or better yet a pull request :)

# About

Uberducky was written by Mike Ryan of [ICE9 Consulting](https://ice9.us). The
author disclaims any liability for the illegal use of this tool.
