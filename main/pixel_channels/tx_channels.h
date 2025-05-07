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

#ifndef SUPER_SCORPIO_TX_CHANNELS_H
#define SUPER_SCORPIO_TX_CHANNELS_H

#include "common.h"
#include "pixel_tx_loop/tx_pixels.h"
#include "pixel_feeds/pixel_feeds.h"
#include "pixel_tx_loop/tx_bytes_feed.h"
#include "channel_layouts/channel_layouts.h"

typedef struct tx_channel {
    uint8_t tx_channel_num;
    uint8_t gpio_num;
    uint8_t sm_num;
    pixel_type_t pixel_type;
    uint32_t gpio_mask;

    channel_layout_t layout;
    uint16_t pixel_count;
    uint16_t chain_offset;
    uint16_t chain_index;

    pixel_feed_t root_feed;
    int32_t bytes_fed_ready_interval;

    tx_status_t tx_status;
} tx_channel_t;


// usage: tx_channels[tx_channel_num]
extern tx_channel_t tx_channels[NUM_TX_PINS];

// usage: gpio_tx_channels[gpio_num]
extern tx_channel_t * const gpio_tx_channels;

void init_tx_channels();

#endif //SUPER_SCORPIO_TX_CHANNELS_H
