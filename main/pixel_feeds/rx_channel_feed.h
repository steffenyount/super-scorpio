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

#ifndef SUPER_SCORPIO_RX_CHANNEL_FEED_H
#define SUPER_SCORPIO_RX_CHANNEL_FEED_H

#include "pixel_feed_base.h"

typedef struct rx_channel rx_channel_t;

typedef struct rx_channel_feed {
    pixel_feed_base_t;
    rx_channel_t * rx_chan;
} rx_channel_feed_t;


void set_rx_channel_feed_chain(uint8_t count, const uint8_t tx_gpio_nums[count], uint8_t rx_channel_num, uint16_t chain_offset);

static inline void set_rx_channel_feed_with_offset(uint8_t tx_gpio_num, uint8_t rx_channel_num, uint16_t start_offset) {
    set_rx_channel_feed_chain(1, (uint8_t [1]) {tx_gpio_num}, rx_channel_num, start_offset);
}

static inline void set_rx_channel_feed(uint8_t tx_gpio_num, uint8_t rx_channel_num) {
    set_rx_channel_feed_chain(1, (uint8_t [1]) {tx_gpio_num}, rx_channel_num, 0u);
}

#endif //SUPER_SCORPIO_RX_CHANNEL_FEED_H
