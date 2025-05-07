/**
 * Copyright (C) 2024 Steffen Yount
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SUPER_SCORPIO_RX_CHANNELS_H
#define SUPER_SCORPIO_RX_CHANNELS_H

#include "common.h"

#define RX_BYTES_BUFFER_SIZE (512u * 3u)
#define RX_GRB_PIXEL_BUFFER_SIZE (512u)
#define RX_RGBW_PIXEL_BUFFER_SIZE (384u)
//#define RX_BYTES_BUFFER_SIZE (64u * 3u)
//#define RX_GRB_PIXEL_BUFFER_SIZE (64u)
//#define RX_RGBW_PIXEL_BUFFER_SIZE (48u)


typedef struct rx_channel {
    uint8_t rx_channel_num;
    uint8_t gpio_num;
    pixel_type_t pixel_type;
    uint8_t reserved;
    union {
        uint8_t bytes[RX_BYTES_BUFFER_SIZE];
        grb_pixel_t grb_pixels[RX_GRB_PIXEL_BUFFER_SIZE];
        rgbw_pixel_t rgbw_pixels[RX_RGBW_PIXEL_BUFFER_SIZE];
    };
    uint16_t byte_count;
    uint16_t pixel_count;
    volatile uint16_t frame_count;
    uint16_t bytes_fed;
    uint32_t tx_feed_triggers;
} rx_channel_t;

extern rx_channel_t rx_channels[NUM_RX_PINS];

void init_rx_channels();

void dump_new_rx_pixels_buffers();

#endif //SUPER_SCORPIO_RX_CHANNELS_H
