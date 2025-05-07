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

#include "rx_bytes_feeds.h"
#include "pixel_channels/rx_channels.h"

#include "hardware/dma.h"
#include "hardware/pio.h"


void init_dma_sm_rx_bytes_feed(uint sm, uint32_t feed_channel) {
    dma_channel_claim(feed_channel);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, false);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, pio_get_dreq(pio0, sm, false));
    channel_config_set_chain_to(&dma_channel_ctrl, feed_channel); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_8);
    channel_config_set_ring(&dma_channel_ctrl, true, 0u);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, true);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(feed_channel, &dma_channel_ctrl, false);

    dma_channel_set_read_addr(feed_channel, &pio0->rxf[sm], false);
    dma_channel_set_write_addr(feed_channel, rx_channels[sm].bytes, false);
    dma_channel_set_trans_count(feed_channel, RX_BYTES_BUFFER_SIZE, true);
}

void init_dma_rx_bytes_feeds() {
    init_dma_sm_rx_bytes_feed(0, DMA_SM0_RX_BYTES_FEED_CHAN);
    init_dma_sm_rx_bytes_feed(1, DMA_SM1_RX_BYTES_FEED_CHAN);
    init_dma_sm_rx_bytes_feed(2, DMA_SM2_RX_BYTES_FEED_CHAN);
    init_dma_sm_rx_bytes_feed(3, DMA_SM3_RX_BYTES_FEED_CHAN);
}