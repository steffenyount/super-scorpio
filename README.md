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

## Hardware: The Super Scorpio ARGB Controller Hub
The Super Scorpio ARGB controller hub consists of an off-the-shelf Adafruit Feather RP2040 Scorpio board paired with my
custom Super Scorpio FeatherWing board.

### Adafruit's Feather RP2040 Scorpio board
The Adafruit Feather RP2040 Scorpio board includes a USB Type-C connector for powering and programing the RP2040
processor, 264KB of SRAM, 8MB of SPI Flash, a 12MHz crystal to run at a reliable 125 MHz, and 8 level shifters with
dedicated pins to control 8 channels of 5V ARGB data output.

https://learn.adafruit.com/introducing-feather-rp2040-scorpio
https://github.com/adafruit/Adafruit-Feather-RP2040-SCORPIO-PCB

![Adafruit Feather RP2040 Scorpio](assets/images/RP2040-Scorpio.jpg)

### My custom Super Scorpio FeatherWing board
My custom Super Scorpio FeatherWing board mounts on top of the Adafruit Feather RP2040 Scorpio board, adding a 5V power
bus, a current sensor, level shifters, and many more pins for 5V ARGB I/O connections using the standard compact VDG pin
layout.

My EasyEDA project for the [Super Scorpio FeatherWing board](https://oshwlab.com/steffenyount/drgb-controller_copy_copy_copy_copy)

![Super Scorpio FeatherWing board](assets/images/Super_Scorpio_FeatherWing_v3_small.jpg)

### Features
The Super Scorpio ARGB Controller Hub includes the following hardware features:
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


## Software: The Super Scorpio driver
The Super Scorpio driver source code is organized into modules each with its own operational focus.

### Fast systick logging
#### [tick_log](main/tick_log/)

### Power monitoring
#### [power_monitor](main/power_monitor/)

### LED segment discovery
#### [channel_control](main/channel_control/)
#### [channel_discovery](main/channel_discovery/)

### Power limiting
#### [power_limiter](main/power_limiter/)

### Pixel channels
#### [pixel_channels](main/pixel_channels/)
#### [channel_layouts](main/channel_layouts/)
#### [channel_overrides](main/channel_overrides/)

### RX/TX offloading
#### [pixel_rx_loop](main/pixel_rx_loop/)
#### [pixel_tx_loop](main/pixel_tx_loop/)

### Pixel feeds
#### [pixel_feeds](main/pixel_feeds/)

### The startup sequence
#### [main.c](main/main.c)

### The runtime loop

## Project status
### This project is still a work in progress
The core functionality is there. LED segment discovery is working, and the runtime loop reliably generates or relays 16
channels x 800Kbps of simultaneous ARGB data. Custom channel layouts can be configured statically using compile time
overrides.

### New features under consideration:
* Need a better way to configure/rotate/map individual parts of daisy chained pixel segments on a shared channel.
* Need to revisit the use of the rgbw_pixel_t type in the tx_pixels[][] buffer and probably the channel_layouts
abstraction. I'd like to support mixing both GRB and RGB 3-byte pixel segments on a shared channel. With this change
the tx_pixels[][] buffer semantics will change to contain either a 3-byte (or 4-byte) array of presorted GRB or RGB
pixel bytes, ready for DMA to push out to the PIO in the target pixel segment's expected byte order.
* Explore a compound/blending pixel_feed type using the RP2040's interpolator hardware.
* Would be nice to be able to specify pixel segment layouts in a 2D canvas, and then use the canvas's coordinates to
inform pixel data selection for a segment's pixel feed. A bonus feature could lean on the interpolator hardware to
rescale/map canvas pixels from a 2D input frame's pixel coordinates.
* Investigate building a web based control interface over the USB connection.
* Implement runtime persistence in flash for channel configs and palette based pixel feeds (should survive software
updates)
* Possibly explore a new hardware/software implementation based on RP2350B for other use cases? With 3 PIO blocks
dedicated to TX, this could theoretically drive 48 ARGB channels concurrently. Would this work as an HDMI adapter?
Could this drive a 128x72 ARGB pixel display composed of 36 individual 32x8 panels at 60fps? Or drive a 256x144 ARGB
pixel display composed of 144 individual 16x16 panels chained with 3 segments per channel at 30fps?

