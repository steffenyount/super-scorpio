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

#ifndef SUPER_SCORPIO_TX_PIXELS_H
#define SUPER_SCORPIO_TX_PIXELS_H

#include "common.h"

#define NUM_TX_PIXEL_BUFFERS (2u)

extern uint32_t tx_pixels_enabled[NUM_TX_PIXEL_BUFFERS];

// usage: tx_pixels[buffer_index][tx_channel_num]
extern rgbw_pixel_t tx_pixels[NUM_TX_PIXEL_BUFFERS][NUM_TX_PINS];

extern uint32_t grb_3_channel_dest_buffer_index;
extern uint32_t rgbw_4_channel_dest_buffer_index;

extern uint32_t grb_3_channel_src_buffer_index;
extern uint32_t rgbw_4_channel_src_buffer_index;

static inline void advance_grb_3_channel_dest_buffer_index() {
    grb_3_channel_src_buffer_index = grb_3_channel_dest_buffer_index;
    grb_3_channel_dest_buffer_index ^= 0x00000001u;
}

static inline void advance_rgbw_4_channel_dest_buffer_index() {
    rgbw_4_channel_src_buffer_index = rgbw_4_channel_dest_buffer_index;
    rgbw_4_channel_dest_buffer_index ^= 0x00000001u;
}

#endif //SUPER_SCORPIO_TX_PIXELS_H
