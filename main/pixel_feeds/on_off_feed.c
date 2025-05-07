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

#include "on_off_feed.h"
#include "pixel_channels/tx_channels.h"
#include "pixel_tx_loop/pixel_tx_loop.h"
#include "pixel_tx_loop/tx_bytes_feed.h"
#include "pixel_channels/rx_channels.h"

static void on_off_feed__open_frame(pixel_feed_t * const this_feed) {
    // no-op
}

static void on_off_feed__feed_pixel(pixel_feed_t * const this_feed, rgbw_pixel_t * const rgbw_dest) {
    on_off_feed_t * const this = &this_feed->on_off;
    uint16_t curr_pixel = this->tx_chan->chain_index;
    uint32_t curr_frame = this->chain_root->tx_status.frames_fed;

    // pattern repeats every 2^6 bits => every 64 pixels
    uint16_t pos = curr_frame + curr_pixel;
    bool on_off_feed_value = !(pos & 0x20u);  // !(bit 5)
    uint16_t idx = (pos >> 3) & 0x3u;         // bits 3..4
    uint8_t value = 1u << (pos & 0x07u);      // bits 0..2
    if (0u == idx) {
        *rgbw_dest = (rgbw_pixel_t) {
                .blue = on_off_feed_value ? value : 0x00,
                .red = on_off_feed_value ? value : 0x00,
                .green = on_off_feed_value ? value : 0x00,
                .white = on_off_feed_value ? value : 0x00,
        };

        if (on_off_feed_value && this->tx_chan->chain_offset == curr_pixel) {
            this->src_pixel.uint32 = rgbw_dest->uint32;
        }

    } else if (1u == idx) {
        *rgbw_dest = (rgbw_pixel_t) {
                .blue = 0x00,
                .red = 0x00,
                .green = on_off_feed_value ? value : 0x00,
                .white = 0x00,
        };

        if (on_off_feed_value && this->tx_chan->chain_offset == curr_pixel) {
            this->src_pixel.uint32 = rgbw_dest->uint32;
        }

    } else if (2u == idx) {
        *rgbw_dest = (rgbw_pixel_t) {
                .blue = 0x00,
                .red = on_off_feed_value ? value : 0x00,
                .green = 0x00,
                .white = 0x00,
        };

        if (on_off_feed_value && this->tx_chan->chain_offset == curr_pixel) {
            this->src_pixel.uint32 = rgbw_dest->uint32;
        }

    } else /* if (3u == idx) */ {
        *rgbw_dest = (rgbw_pixel_t) {
                .blue = on_off_feed_value ? value : 0x00,
                .red = 0x00,
                .green = 0x00,
                .white = 0x00,
        };

        if (on_off_feed_value && this->tx_chan->chain_offset == curr_pixel) {
            this->src_pixel.uint32 = rgbw_dest->uint32;
        }
    }
}

static void on_off_feed__close_frame(pixel_feed_t * const this_feed) {
    // no-op
}

static inline void set_on_off_feed_with_chain_root(tx_channel_t * const tx_chan, tx_channel_t * const chain_root) {
    tx_chan->root_feed.on_off = (on_off_feed_t) {
            .open_frame = on_off_feed__open_frame,
            .feed_pixel = on_off_feed__feed_pixel,
            .close_frame = on_off_feed__close_frame,
            .tx_chan = tx_chan,
            .chain_root = chain_root,
    };

    // ensure rx_chan triggers are disabled
    for (uint32_t rx_channel_num = 0u; rx_channel_num < NUM_RX_PINS; rx_channel_num++) {
        clear_bits(&rx_channels[rx_channel_num].tx_feed_triggers, tx_chan->gpio_mask);
    }
    // enable bytes_fed trigger for every 0.1 second
    tx_chan->bytes_fed_ready_interval = 50000u; //10000u ;
}

void set_on_off_feed_chain(uint8_t count, const uint8_t tx_gpio_nums[count], uint16_t chain_offset) {
    tx_channel_t * chain_root = NULL;

    for (uint8_t ii = 0u; ii < count; ii++) {
        tx_channel_t * const tx_chan = &gpio_tx_channels[tx_gpio_nums[ii]];
        if (tx_chan->pixel_count) {
            if (!chain_root) {
                chain_root = tx_chan;
            }
            set_on_off_feed_with_chain_root(tx_chan, chain_root);

            tx_chan->chain_offset = chain_offset;
            chain_offset += tx_chan->pixel_count;

        } else {
            set_empty_feed(tx_gpio_nums[ii]);
        }
    }
}