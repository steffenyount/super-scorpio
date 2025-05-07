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
#include "power_monitor_feed.h"
#include "power_samples.h"

#include "hardware/adc.h"

static bool power_monitor_feed_initialized = false;
static bool power_monitor_feed_trigger_initialized = false;

static volatile uint32_t channel_trigger_mask = DMA_POWER_MONITOR_FEED_CHAN_MASK;

static inline void init_power_monitor_feed() {
    if (!power_monitor_feed_initialized) {
        dma_channel_claim(DMA_POWER_MONITOR_FEED_CHAN);
        power_monitor_feed_initialized = true;
    }

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, false);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_ADC);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_POWER_MONITOR_FEED_TRIGGER_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_16);
    channel_config_set_ring(&dma_channel_ctrl, true, POWER_SAMPLES_ALIGN_SIZE_BITS);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_POWER_MONITOR_FEED_CHAN, &dma_channel_ctrl, false);

    dma_channel_set_read_addr(DMA_POWER_MONITOR_FEED_CHAN, &adc_hw->fifo, false);
    dma_channel_set_write_addr(DMA_POWER_MONITOR_FEED_CHAN, power_samples, false);
    dma_channel_set_trans_count(DMA_POWER_MONITOR_FEED_CHAN, POWER_SAMPLES_SIZE, false);
}

static inline void init_power_monitor_feed_trigger() {
    if (!power_monitor_feed_trigger_initialized) {
        dma_channel_claim(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN);
        power_monitor_feed_trigger_initialized = true;
    }
    power_monitor_feed_trigger_set_config(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN); // self == disable

    channel_trigger_mask = DMA_POWER_MONITOR_FEED_CHAN_MASK;

    dma_channel_set_read_addr(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN, &channel_trigger_mask, false);
    dma_channel_set_write_addr(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN, &dma_hw->multi_channel_trigger, false);
    dma_channel_set_trans_count(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN, 1, false);
}

static inline void trigger_power_monitor_feed() {
    dma_hw->multi_channel_trigger = DMA_POWER_MONITOR_FEED_CHAN_MASK;
}

void start_power_monitor_feed() {
    init_power_monitor_feed();
    init_power_monitor_feed_trigger();
    trigger_power_monitor_feed();
}

void stop_power_monitor_feed() {
    channel_trigger_mask = 0u;
    dma_channel_wait_for_finish_blocking(DMA_POWER_MONITOR_FEED_CHAN);
}
