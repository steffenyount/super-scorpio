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

#ifndef SUPER_SCORPIO_POWER_MONITOR_FEED_H
#define SUPER_SCORPIO_POWER_MONITOR_FEED_H

#include "power_monitor.h"

#include "hardware/dma.h"


static inline void power_monitor_feed_trigger_set_config(uint chain_to) {
    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, false);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, chain_to); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 0u);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN, &dma_channel_ctrl, false);
}


void start_power_monitor_feed();

void stop_power_monitor_feed();

#endif //SUPER_SCORPIO_POWER_MONITOR_FEED_H
