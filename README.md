# Welcome to the Super Scorpio project!
Chain and control multiple NeoPixel/ARGB LED strips with the Super Scorpio!

## Background
I was looking for an excuse to work with the RP2040 microcontroller and I found one. My computer case "needed" some RGB
bling, and as I added addressable RGB kits here and there for the pumps, plates, and fans, etc, I quickly found out
that most-all off-the-shelf ARGB kits have zero provisions for daisy chaining their individual ARGB segments. This is
unfortunate since my motherboard provides just 1 ARGB header while my video card also provides just 1 ARGB header. Both
of these ARGB sources have hardware to make pretty blinken-lights while also color encoding some hardware state
information like CPU/GPU temps, etc.

True, there are some off-the shelf ARGB controller hubs out there, but none did what I wanted, so the Super Scorpio
project was born. With my Super Scorpio I can now read the two ARGB data streams from both my motherboard and my video
card and then arbitrarily map their pixel data outputs onto all of my separate kits' individual ARGB LED segments.

## The Hardware
### The Super Scorpio ARGB Controller Hub
The Super Scorpio ARGB controller hub consists of an off-the-shelf Adafruit Feather RP2040 Scorpio board paired with my
custom Super Scorpio FeatherWing board.

#### Adafruit's Feather RP2040 Scorpio board
The Adafruit Feather RP2040 Scorpio board includes a USB Type-C connector for powering and programing the RP2040
processor, 264KB of SRAM, 8MB of SPI Flash, a 12MHz crystal to run at a reliable 125 MHz, and 8 level shifters with
dedicated pins to control 8 channels of 5V ARGB output.

https://learn.adafruit.com/introducing-feather-rp2040-scorpio
https://github.com/adafruit/Adafruit-Feather-RP2040-SCORPIO-PCB

![Adafruit Feather RP2040 Scorpio](assets/images/RP2040-Scorpio.jpg)

#### My custom Super Scorpio FeatherWing board
My custom Super Scorpio FeatherWing board mounts on top of the Adafruit Feather RP2040 Scorpio board, adding a 5V power
bus, a current sensor, level shifters, and many more pins for 5V ARGB I/O connections using a standard compact VDG pin
layout.

My EasyEDA project for the [Super Scorpio FeatherWing PCB](https://oshwlab.com/steffenyount/drgb-controller_copy_copy_copy_copy)

![Super Scorpio FeatherWing board](assets/images/Super_Scorpio_FeatherWing_v3_small.jpg)

#### Features
The Super Scorpio ARGB Controller Hub includes the following features:
* Molex power connector to deliver 50W of LED power from an ATX power supply
* LED power/current monitoring over a 0-11A input range (ADC pin A0)
* LED power and data I/O connectivity provided with compact DG/VDG pin headers
* 4x level shifted 5V ARGB data input channels (GPIO pins 5,6,24,25)
* 16x level shifted 5V ARGB data output channels (GPIO pins 8-23)
* 264KB of SRAM
* 8MB of SPI Flash
* 2x 32-bit Cortex M0+ cores running at ~125 MHz
* 2x PIO blocks (RP2040 programmable input/output processors) for ARGB data I/O offloading
* 12x DMA controllers

## The Software
