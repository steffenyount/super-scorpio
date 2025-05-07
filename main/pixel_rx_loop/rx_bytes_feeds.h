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

#ifndef SUPER_SCORPIO_RX_BYTES_FEEDS_H
#define SUPER_SCORPIO_RX_BYTES_FEEDS_H

#include "pixel_rx_loop.h"
#include "pixel_channels/rx_channels.h"

#include "hardware/dma.h"

void init_dma_rx_bytes_feeds();

static inline void start_dma_sm_rx_bytes_feed(uint sm, uint32_t feed_channel) {
    dma_channel_set_write_addr(feed_channel, rx_channels[sm].bytes, true);
}

static inline uint32_t get_sm_rx_bytes_fed(uint32_t feed_channel) {
    return RX_BYTES_BUFFER_SIZE - dma_hw->ch[feed_channel].transfer_count;
}

#endif //SUPER_SCORPIO_RX_BYTES_FEEDS_H
