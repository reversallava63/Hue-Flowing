<<<<<<< HEAD
# Hue-Flowing
Hue Flowing port to the Numworks calculator
=======
# Nofrendo

This app is a [NES](https://en.wikipedia.org/wiki/Nintendo_Entertainment_System) emulator that runs on the [NumWorks calculator](https://www.numworks.com).

Available on [Nwagyu](https://yaya-cout.github.io/Nwagyu/)

## Install the app

To install this app, you'll need to:

1. Download the latest `nofrendo.nwa` file from the [Releases](https://codeberg.org/Yaya-Cout/nofrendo/releases) page
2. Extract a `cartridge.nes` ROM dump from your NES cartridge, or, alternatively, use the provided `src/2048.nes` file.
3. Head to [my.numworks.com/apps](https://my.numworks.com/apps) to send the `nwa` file on your calculator along the `nes` file.

## How to use the app

The controls are pretty obvious because the NES gamepad looks a lot like the NumWorks' keyboard:

|NES controls|NumWorks|
|-|-|
|Arrow|Arrows|
|B|Back|
|A|OK|
|Select|Shift|
|Start|Backspace|
|Reset|Tangent|
|Save state|Square Root|
|Save state and exit|0|

## Build the app

To build this sample app, you will need to install the [embedded ARM toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) and [nwlink](https://www.npmjs.com/package/nwlink).

```shell
brew install numworks/tap/arm-none-eabi-gcc node # Or equivalent on your OS
npm install -g nwlink
make clean && make build
```
>>>>>>> e3d3c89 (Save local changes before rebase)
