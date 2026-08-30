
Hue Flowing port to the Numworks calculator
=======

A faithful C port of the Pygame game **Hue Flowing** by DaFluffyPotato (https://dafluffypotato.itch.io/hue-flowing), designed for the **NumWorks N0120 graphing calculator** using the EADK SDK.

The game features a unique painting mechanic where the world is hidden behind a white canvas and is gradually revealed as the player moves.

## Install the app

To install this app, you'll need to:

1. Download the latest `hue.nwa` file from the [Releases](https://github.com/reversallava63/Hue-Flowing/releases/) page
2. Head to [my.numworks.com/apps](https://my.numworks.com/apps) to send the `nwa` file on your calculator.

# Roadmap

- [x] Full world rendering – 197×99 tile map, decor, animated foliage, entities
- [x] Persistent world‑space canvas/paint system
- [x] Player physics: gravity, acceleration, wall‑slide, wall‑jump, double‑jump, respawn
- [x] Player animations: idle, run, jump, slide, rotation
- [x] Jump icon overlay showing available/used jumps
- [x] Soft mask borders for a polished canvas effect
- [x] Memory‑optimised chunked world mask and line‑buffer rendering

## How to use the app

Controls:

|Game control|NumWorks keyboard|
|-|-|
|Go Left|Left Arrow|
|Go Right|Right Arrow|
|Jump|OK|

## Assets

Convert original Pygame assets to C headers using the provided scripts

## Technical assets

Memory – Chunked 2‑bit world mask with 4×4 downsampling (~47 KB for the whole level)

Rendering – Scanline‑based line buffer (320 pixels wide)

Performance – Quarter‑resolution background cache and precomputed sprite bounds


## Build the app

To build this sample app, you will need to install the [embedded ARM toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) and [nwlink](https://www.npmjs.com/package/nwlink).

```shell
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi nodejs npm #Or equivalent on your OS
git clone https://github.com/yourusername/hue-flowing-numworks.git
cd hue-flowing-numworks
make clean && make build
```

