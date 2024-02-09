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

#include "pwm_cc_feed_trigger.h"
#include "pwm_cc_feed_director.h"

#include "hardware/pwm.h"


static uint32_t __scratch_y("super_scorpio") __alignment(5) * feed_srcs_for_dma_pwm_cc_feed_director[8] = {
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed,
(uint32_t *)rxf_dests_for_dma_pwm_cc_feed_last
};

// dma_pma_cc_feed_trigger: dma_pwm_cc_feed_director__feed_sequence_srcs -> dma_pwm_cc_feed_director read_addr_trig
void __not_in_flash_func(init_dma_pwm_cc_feed_trigger)() {
    dma_channel_claim(DMA_PWM_CC_FEED_TRIGGER_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, pwm_get_dreq(PWM_SENTINEL_SLICE));
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_FEED_TRIGGER_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, false, 5);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_FEED_TRIGGER_CHAN, &dma_channel_ctrl, false);

    dma_channel_set_read_addr(DMA_PWM_CC_FEED_TRIGGER_CHAN, feed_srcs_for_dma_pwm_cc_feed_director, false);
    dma_channel_set_write_addr(DMA_PWM_CC_FEED_TRIGGER_CHAN, &dma_hw->ch[DMA_PWM_CC_FEED_DIRECTOR_CHAN].al3_read_addr_trig, false);
    dma_channel_set_trans_count(DMA_PWM_CC_FEED_TRIGGER_CHAN, 8, false);
}
