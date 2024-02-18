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

#include "pwm_cc_feed_discard.h"

#include "hardware/pio.h"
#include "hardware/pwm.h"

static uint32_t __not_in_flash("super_scorpio") discard;

// dma_pwm_cc_feed_diacard: pio1->rxf values -> discard;
void __not_in_flash_func(init_dma_pwm_cc_feed_discard)() {
    dma_channel_claim(DMA_PWM_CC_FEED_DISCARD_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_FEED_DISCARD_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, false, 4); // reading from 4x rxf x 4-bytes = 16
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_FEED_DISCARD_CHAN, &dma_channel_ctrl, false);

    dma_channel_set_read_addr(DMA_PWM_CC_FEED_DISCARD_CHAN, pio1->rxf, false);
    dma_channel_set_write_addr(DMA_PWM_CC_FEED_DISCARD_CHAN, &discard, false);
    dma_channel_set_trans_count(DMA_PWM_CC_FEED_DISCARD_CHAN, 16u, false);
}