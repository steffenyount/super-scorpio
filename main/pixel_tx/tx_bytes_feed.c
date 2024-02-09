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

#include "tx_bytes_feed.h"

#include "hardware/pio.h"

// 8 PWM_SLICES x 2 PWM_CHANNELS x 2 bytes (curr & enable) = 32
#define PWM_SLICE_TX_BYTES_ALIGN_SIZE_BITS (5u)


pwm_slice_tx_bytes_t __scratch_y("super_scorpio") __alignment(PWM_SLICE_TX_BYTES_ALIGN_SIZE_BITS) pwm_slice_tx_bytes[NUM_PWM_SLICES];
tx_byte_t __scratch_y("super_scorpio") * gpio_tx_bytes = ((tx_byte_t *) &pwm_slice_tx_bytes[0]) - GPIO_TX_PINS_BEGIN;

volatile bool __scratch_y("super_scorpio") tx_bytes_feed_done = false;

// dma_tx_bytes_feed: pwm_slice_tx_bytes -> pio1 TX FIFOs (tx_bytes_to_pwm_cc) & trigger dma_pio_rxf_ready_trigger
void __not_in_flash_func(init_dma_tx_bytes_feed)() {
    dma_channel_claim(DMA_TX_BYTES_FEED_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_BYTES_FEED_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 4);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_TX_BYTES_FEED_CHAN, &dma_channel_ctrl, false);

    //    dma_channel_set_read_addr(DMA_TX_BYTES_FEED_CHAN, &pwm_slice_tx_bytes[0], false);
    dma_channel_set_write_addr(DMA_TX_BYTES_FEED_CHAN, pio1->txf, false);
    dma_channel_set_trans_count(DMA_TX_BYTES_FEED_CHAN, NUM_PWM_SLICES, false);
}
