# gpiemu
Bare metal emulators and launcher for retroflag GPI v1

Bare metal environment provided by the circle-stdlib project.

https://github.com/smuehlst/circle-stdlib

## GPi Case Configuration

Your GPi Case must be in "Hat" input mode for correct operation.

If you're not able to navigate menus, set the input mode by holding **Up** on the D-Pad and **Select** for around five seconds, until the power LED flashes purple. Depending on your exact model of GPi Case, you may need to hold **Up** and **Start** instead.

## Build Instructions

note: Toolchain setup based on circle-stdlib advice, copying for reference

export GPITOOLCHAIN=/home/dave/Toolchain/arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi
export GPISDKINSTALL=/home/dave/Documents/projects/circle-stdlib/install