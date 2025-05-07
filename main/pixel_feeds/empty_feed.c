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

#include "empty_feed.h"
#include "pixel_channels/tx_channels.h"
#include "pixel_channels/rx_channels.h"


static void empty_feed__open_frame(pixel_feed_t * const this_feed) {
}

static void empty_feed__feed_pixel(pixel_feed_t * const this_feed, rgbw_pixel_t * const rgbw_dest) {

}

static void empty_feed__close_frame(pixel_feed_t * const this_feed) {
    // no-op
}

empty_feed_t empty_feed = {
        .open_frame = empty_feed__open_frame,
        .feed_pixel = empty_feed__feed_pixel,
        .close_frame = empty_feed__close_frame,
        .tx_chan = NULL,
        .chain_root = NULL,
};

void set_empty_feed(uint8_t gpio_num) {
    tx_channel_t * const tx_chan = &gpio_tx_channels[gpio_num];

    tx_chan->root_feed.empty = empty_feed;

    // ensure rx_chan triggers are disabled
    for (uint32_t rx_channel_num = 0u; rx_channel_num < NUM_RX_PINS; rx_channel_num++) {
        clear_bits(&rx_channels[rx_channel_num].tx_feed_triggers, tx_chan->gpio_mask);
    }
    // ensure bytes_fed trigger is disabled
    tx_chan->bytes_fed_ready_interval = 0u;
}