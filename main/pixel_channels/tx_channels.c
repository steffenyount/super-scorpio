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

#include "tx_channels.h"
#include "pixel_feeds/empty_feed.h"
#include "channel_layouts/linear_layout.h"


tx_channel_t __alignment(2) tx_channels[NUM_TX_PINS];
tx_channel_t * const gpio_tx_channels = &tx_channels[0] - GPIO_TX_PINS_BEGIN;

void init_tx_channels() {
    for (uint32_t ii = 0u; ii < NUM_TX_PINS; ii++) {
        uint8_t gpio_num = GPIO_TX_PINS_BEGIN + ii;

        tx_channels[ii] = (tx_channel_t) {
                .tx_channel_num = ii,
                .gpio_num = gpio_num,
                .gpio_mask = 1u << gpio_num,
                .sm_num = ii >> 2,
                .pixel_type = GRB_3_BYTE,
                .layout = linear_layout,
                .pixel_count = 0u,
                .chain_offset = 0u,
                .chain_index = 0u,
                .root_feed.empty = empty_feed,
                .bytes_fed_ready_interval = 0u,
                .tx_status = {
                        .pixels_fed = 0u,
                        .reserved = 0u,
                        .frames_fed = 0u,
                }
        };
    }
}
