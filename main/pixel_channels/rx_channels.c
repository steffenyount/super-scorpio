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

#include "rx_channels.h"
#include <pico/printf.h>


rx_channel_t __alignment(2) rx_channels[NUM_RX_PINS];


void init_rx_channels() {
    for (uint32_t ii = 0u; ii < NUM_RX_PINS; ii++) {
        rx_channels[ii] = (rx_channel_t) {
            .rx_channel_num = ii,
            .gpio_num = rx_gpio_nums[ii],
            .pixel_type = GRB_3_BYTE,
            .byte_count = 0u,
            .pixel_count = 0u,
            .frame_count = 0u,
            .bytes_fed = RX_BYTES_BUFFER_SIZE,
            .tx_feed_triggers = 0u,
        };
    }
}

static uint16_t rx_pixels_buffers_print_prev_update_count[NUM_RX_PINS] = { 0u };

void dump_new_rx_pixels_buffers() {
    printf("dump_new_rx_pixels_buffers()\n");
    for (uint32_t rx_channel_num = 0u; rx_channel_num < NUM_RX_PINS; rx_channel_num++) {
        rx_channel_t * const rx_chan = &rx_channels[rx_channel_num];

        if (rx_pixels_buffers_print_prev_update_count[rx_channel_num] != rx_chan->frame_count) {
            rx_pixels_buffers_print_prev_update_count[rx_channel_num] = rx_chan->frame_count;

            printf("rx_pix[%d]: byte_count %4d pixel_count %3d pixel_type %d update_count %d\n", rx_channel_num,
                   rx_chan->byte_count, rx_chan->pixel_count, rx_chan->pixel_type, rx_chan->frame_count);
            if (GRB_3_BYTE == rx_chan->pixel_type) {
                for (uint32_t idx = 0; idx < rx_chan->pixel_count; idx++) {
                    if(0 == (idx & 3)) {
                        printf("\nrx_pix[%d].grb[%3d] ", rx_channel_num, idx);
                    }
                    printf("%02x %02x %02x  ",
                           rx_chan->grb_pixels[idx].green, rx_chan->grb_pixels[idx].red, rx_chan->grb_pixels[idx].blue);
                }
                printf("\n");
            } else {
                for (uint32_t idx = 0; idx < rx_chan->pixel_count; idx++) {
                    if(0 == (idx % 3)) {
                        printf("\nrx_pix[%d].rgbw[%3d] ", rx_channel_num, idx);
                    }
                    printf("%02x %02x %02x %02x  ",
                           rx_chan->rgbw_pixels[idx].red, rx_chan->rgbw_pixels[idx].green,
                           rx_chan->rgbw_pixels[idx].blue, rx_chan->rgbw_pixels[idx].white);
                }
                printf("\n");
            }
        }
    }
    printf("dump_new_rx_pixels_buffers() - done\n");
}