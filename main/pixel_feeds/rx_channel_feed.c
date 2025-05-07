#include "pixel_channels/tx_channels.h"

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

#include "rx_channel_feed.h"
#include "pixel_channels/rx_channels.h"

static void rx_channel_feed__open_frame(pixel_feed_t * const this_feed) {
    // no-op
}

static void rx_channel_feed__feed_pixel(pixel_feed_t * const this_feed, rgbw_pixel_t * const rgbw_dest) {
    rx_channel_feed_t * const this = &this_feed->rx_channel;
    tx_channel_t * const tx_chan = this->tx_chan;
    rx_channel_t * const rx_chan = this->rx_chan;

    if (tx_chan->chain_index < rx_chan->pixel_count) {
        switch (rx_chan->pixel_type) {
            case GRB_3_BYTE: {
                grb_pixel_t * const curr_pixel = &rx_chan->grb_pixels[tx_chan->chain_index];
                *rgbw_dest = (rgbw_pixel_t) {
                        .red = curr_pixel->red,
                        .green = curr_pixel->green,
                        .blue = curr_pixel->blue,
                        .white = 0x00u,
                };
                break;
            }
            case RGBW_4_BYTE: {
                rgbw_pixel_t * const curr_pixel = &rx_chan->rgbw_pixels[tx_chan->chain_index];
                *rgbw_dest = (rgbw_pixel_t) {
                        .red = curr_pixel->red,
                        .green = curr_pixel->green,
                        .blue = curr_pixel->blue,
                        .white = curr_pixel->white,
                };
                break;
            }
            default: {
                rgbw_dest->uint32 = rgbw_pixel_off.uint32;
                break;
            }
        }
    } else {
        rgbw_dest->uint32 = rgbw_pixel_off.uint32;
    }
}

static void rx_channel_feed__close_frame(pixel_feed_t * const this_feed) {
    // no-op
}

static inline void set_rx_channel_feed_with_chain_root(tx_channel_t * const tx_chan, tx_channel_t * const chain_root, const uint8_t rx_channel_num) {
    tx_chan->root_feed.rx_channel = (rx_channel_feed_t) {
            .open_frame = rx_channel_feed__open_frame,
            .feed_pixel = rx_channel_feed__feed_pixel,
            .close_frame = rx_channel_feed__close_frame,
            .tx_chan = tx_chan,
            .chain_root = chain_root,
            .rx_chan = &rx_channels[rx_channel_num],
    };

    // ensure only the rx_chan trigger is enabled
    for (uint32_t rx_channel_idx = 0u; rx_channel_idx < NUM_RX_PINS; rx_channel_idx++) {
        if (rx_channel_idx == rx_channel_num) {
            set_bits(&rx_channels[rx_channel_idx].tx_feed_triggers, tx_chan->gpio_mask);

        } else {
            clear_bits(&rx_channels[rx_channel_idx].tx_feed_triggers, tx_chan->gpio_mask);
        }
    }
    // ensure bytes_fed trigger is disabled
    tx_chan->bytes_fed_ready_interval = 0u;
}

void set_rx_channel_feed_chain(uint8_t count, const uint8_t tx_gpio_nums[count], const uint8_t rx_channel_num, uint16_t chain_offset) {
    tx_channel_t * chain_root = NULL;

    for (uint8_t ii = 0u; ii < count; ii++) {
        tx_channel_t * const tx_chan = &gpio_tx_channels[tx_gpio_nums[ii]];
        if (tx_chan->pixel_count) {
            if (!chain_root) {
                chain_root = tx_chan;
            }
            set_rx_channel_feed_with_chain_root(tx_chan, chain_root, rx_channel_num);

            tx_chan->chain_offset = chain_offset;
            chain_offset += tx_chan->pixel_count;

        } else {
            set_empty_feed(tx_gpio_nums[ii]);
        }
    }
}