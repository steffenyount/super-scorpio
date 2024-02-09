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

#include "pwm_cc_feed_director.h"

#include "hardware/pio.h"
#include "hardware/pwm.h"

static uint32_t __scratch_y("super_scorpio") discard;

// note: the tx_bytes for PWM slice_nums 0-3 are at pwm_slice_tx_bytes[4..7], and 4-7 are at pwm_slice_tx_bytes[0..3]
uint32_t __scratch_y("super_scorpio") * rxf_dests_for_dma_pwm_cc_feed[17] = {
(uint32_t *)&pio1->txf[0],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[1],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[2],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[3],          // loop back rxf -> txf
(uint32_t *)&pwm_hw->slice[4].cc,
(uint32_t *)&pwm_hw->slice[5].cc,
(uint32_t *)&pwm_hw->slice[6].cc,
(uint32_t *)&pwm_hw->slice[7].cc,

(uint32_t *)&pio1->txf[0],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[1],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[2],          // loop back rxf -> txf
(uint32_t *)&pio1->txf[3],          // loop back rxf -> txf
(uint32_t *)&pwm_hw->slice[0].cc,
(uint32_t *)&pwm_hw->slice[1].cc,
(uint32_t *)&pwm_hw->slice[2].cc,
(uint32_t *)&pwm_hw->slice[3].cc,

NULL
};

uint32_t __scratch_y("super_scorpio") * rxf_dests_for_dma_pwm_cc_feed_last[17] = {
&discard,             // discard loop back rxf value
&discard,             // discard loop back rxf value
&discard,             // discard loop back rxf value
&discard,             // discard loop back rxf value
(uint32_t *)&pwm_hw->slice[4].cc,
(uint32_t *)&pwm_hw->slice[5].cc,
(uint32_t *)&pwm_hw->slice[6].cc,
(uint32_t *)&pwm_hw->slice[7].cc,

&discard,             // ddiscard loop back rxf value
&discard,             // discard loop back rxf value
&discard,             // discard loop back rxf value
&discard,             // discard loop back rxf value
(uint32_t *)&pwm_hw->slice[0].cc,
(uint32_t *)&pwm_hw->slice[1].cc,
(uint32_t *)&pwm_hw->slice[2].cc,
(uint32_t *)&pwm_hw->slice[3].cc,

NULL
};


// dma_pwm_cc_feed_director: rxf_dests_for_dma_pwm_cc_feed -> dma_pwm_cc_feed's write_addr_trig
void __not_in_flash_func(init_dma_pwm_cc_feed_director)() {
    dma_channel_claim(DMA_PWM_CC_FEED_DIRECTOR_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, false, 0);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_FEED_DIRECTOR_CHAN, &dma_channel_ctrl, false);

//    dma_channel_set_read_addr(DMA_PWM_CC_FEED_DIRECTOR_CHAN, rxf_dests_for_dma_pwm_cc_feed, false);
    dma_channel_set_write_addr(DMA_PWM_CC_FEED_DIRECTOR_CHAN, &dma_hw->ch[DMA_PWM_CC_FEED_CHAN].al2_write_addr_trig, false);
    dma_channel_set_trans_count(DMA_PWM_CC_FEED_DIRECTOR_CHAN, 1, false);
}
