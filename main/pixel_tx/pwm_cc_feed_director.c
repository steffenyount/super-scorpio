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
#include "tx_bytes_buffer.h"
#include "tx_bytes_feed_director.h"
#include "pwm_cc_feed_trigger.h"

#include "hardware/pio.h"
#include "hardware/pwm.h"

static uint32_t __not_in_flash("super_scorpio") discard;

volatile uint8_t ** next_srcs_for_tx_bytes_director;


uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__launch_sequence[] = {
        // stage green tx_bytes

        NULL, NULL,

        (uint32_t *)&pwm_slice_tx_bytes[0], (uint32_t *)&pio1->txf[0],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[1], (uint32_t *)&pio1->txf[1],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[2], (uint32_t *)&pio1->txf[2],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[3], (uint32_t *)&pio1->txf[3],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[4], (uint32_t *)&pio1->txf[0],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[5], (uint32_t *)&pio1->txf[1],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[6], (uint32_t *)&pio1->txf[2],  // load green tx_byte
        (uint32_t *)&pwm_slice_tx_bytes[7], (uint32_t *)&pio1->txf[3],  // load green tx_byte

        NULL, NULL,
};

uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__load_tx_bytes_stage_next_tx_bytes_and_feed_pwm_cc_bit_7[20] = {
        (uint32_t *)&pwm_slice_tx_bytes[0], (uint32_t *)&pio1->txf[0],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[1], (uint32_t *)&pio1->txf[1],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[2], (uint32_t *)&pio1->txf[2],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[3], (uint32_t *)&pio1->txf[3],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[4], (uint32_t *)&pio1->txf[0],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[5], (uint32_t *)&pio1->txf[1],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[6], (uint32_t *)&pio1->txf[2],  // load staged tx_bytes into pio
        (uint32_t *)&pwm_slice_tx_bytes[7], (uint32_t *)&pio1->txf[3],  // load staged tx_bytes into pio

        // stage next tx_bytes
        (uint32_t *)&tx_bytes_director_dma_channel_mask, (uint32_t *)&dma_hw->multi_channel_trigger,

        // continue in the __loop_back_tx_bytes_and_feed_pwm_cc_bit sequence
        (uint32_t *)srcs_and_dests_for_pwm_cc_feed_director__loop_back_tx_bytes_and_feed_pwm_cc_bit, (uint32_t *)&dma_hw->ch[DMA_PWM_CC_FEED_DIRECTOR_CHAN].read_addr,
};


// note: the tx_bytes for PWM slice_nums 0-3 are at pwm_slice_tx_bytes[4..7], and 4-7 are at pwm_slice_tx_bytes[0..3]
uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__loop_back_tx_bytes_and_feed_pwm_cc_bit[34] = {
        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pio1->txf[0],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pio1->txf[1],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pio1->txf[2],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pio1->txf[3],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pwm_hw->slice[4].cc,
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pwm_hw->slice[5].cc,
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pwm_hw->slice[6].cc,
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pwm_hw->slice[7].cc,

        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pio1->txf[0],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pio1->txf[1],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pio1->txf[2],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pio1->txf[3],           // loop back rxf -> txf
        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pwm_hw->slice[0].cc,
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pwm_hw->slice[1].cc,
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pwm_hw->slice[2].cc,
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pwm_hw->slice[3].cc,

        NULL, NULL,
};

// note: the tx_bytes for PWM slice_nums 0-3 are at pwm_slice_tx_bytes[4..7], and 4-7 are at pwm_slice_tx_bytes[0..3]
uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__discard_tx_bytes_and_feed_pwm_cc_bit_0[34] = {
        (uint32_t *)&pio1->rxf[0], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[1], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[2], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[3], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pwm_hw->slice[4].cc,
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pwm_hw->slice[5].cc,
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pwm_hw->slice[6].cc,
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pwm_hw->slice[7].cc,

        (uint32_t *)&pio1->rxf[0], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[1], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[2], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[3], &discard,                            // discard done loop back rxf value
        (uint32_t *)&pio1->rxf[0], (uint32_t *)&pwm_hw->slice[0].cc,
        (uint32_t *)&pio1->rxf[1], (uint32_t *)&pwm_hw->slice[1].cc,
        (uint32_t *)&pio1->rxf[2], (uint32_t *)&pwm_hw->slice[2].cc,
        (uint32_t *)&pio1->rxf[3], (uint32_t *)&pwm_hw->slice[3].cc,

        NULL, NULL,
};


uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__source_red_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8] = {{
        // set to feed red tx_bytes from the even tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_red_tx_bytes[0], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

        // update pwm_cc_feed_trigger sequence for next iteration & trigger it
        (uint32_t *)srcs_and_dests_for_pwm_cc_feed_director__source_blue_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[0], (uint32_t *)&srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits[7],
        (uint32_t *)srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits, (uint32_t *)&dma_hw->ch[DMA_PWM_CC_FEED_TRIGGER_CHAN].al3_read_addr_trig,

        // continue in the __discard_tx_bytes_and_feed_pwm_cc_bit_0 sequence
        (uint32_t *)srcs_and_dests_for_pwm_cc_feed_director__discard_tx_bytes_and_feed_pwm_cc_bit_0, (uint32_t *)&dma_hw->ch[DMA_PWM_CC_FEED_DIRECTOR_CHAN].read_addr,

}, {
        // set to feed red tx_bytes from the odd tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_red_tx_bytes[1], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

        // update pwm_cc_feed_trigger sequence for next iteration & trigger it
        (uint32_t *)srcs_and_dests_for_pwm_cc_feed_director__source_blue_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[1], (uint32_t *)&srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits[7],
        (uint32_t *)srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits, (uint32_t *)&dma_hw->ch[DMA_PWM_CC_FEED_TRIGGER_CHAN].al3_read_addr_trig,

        // continue in the __discard_tx_bytes_and_feed_pwm_cc_bit_0 sequence
        (uint32_t *)srcs_and_dests_for_pwm_cc_feed_director__discard_tx_bytes_and_feed_pwm_cc_bit_0, (uint32_t *)&dma_hw->ch[DMA_PWM_CC_FEED_DIRECTOR_CHAN].read_addr,
}};

uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__source_blue_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8] = {{
        // set to feed blue tx_bytes from the even tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_blue_tx_bytes[0], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

        // update pwm_cc_feed_trigger sequence for next iteration & trigger it
        (uint32_t *), (uint32_t *)&srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits[7],


}, {
        // set to feed blue tx_bytes from the odd tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_blue_tx_bytes[1], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

}};

uint32_t __not_in_flash("super_scorpio") * srcs_and_dests_for_pwm_cc_feed_director__source_green_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8] = {{
        // set to feed green tx_bytes from the even tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_green_tx_bytes[0], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

        // update pwm_cc_feed_trigger sequence for next iteration & trigger it
        (uint32_t *), (uint32_t *)&srcs_for_pwm_cc_feed_trigger__feed_8_pwm_cc_bits[7],


}, {
        // set to feed green tx_bytes from the odd tx_pixels buffer
        (uint32_t *)srcs_for_tx_bytes_director__stage_green_tx_bytes[1], (uint32_t *)&dma_hw->ch[DMA_TX_BYTES_FEED_DIRECTOR_CHAN].read_addr,

}};


uint32_t __not_in_flash("super_scorpio") * rxf_srcs_and_dests_for_dma_pwm_cc_feed_shutdown_bit_0[] = {

        NULL, NULL,
};

uint32_t __not_in_flash("super_scorpio") * rxf_srcs_and_dests_for_dma_pwm_cc_feed_shutdown_bits_1_to_6[] = {

        NULL, NULL,
};

uint32_t __not_in_flash("super_scorpio") * rxf_srcs_and_dests_for_dma_pwm_cc_feed_shutdown_bit_7[] = {

        NULL, NULL,
};


// dma_pwm_cc_feed_director: srcs_and_dests_for_pwm_cc_feed_director__loop_back_tx_bytes_and_feed_pwm_cc_bit -> dma_pwm_cc_feed's write_addr_trig
void __not_in_flash_func(init_dma_pwm_cc_feed_director)() {
    dma_channel_claim(DMA_PWM_CC_FEED_DIRECTOR_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 3);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_FEED_DIRECTOR_CHAN, &dma_channel_ctrl, false);

//    dma_channel_set_read_addr(DMA_PWM_CC_FEED_DIRECTOR_CHAN, srcs_and_dests_for_pwm_cc_feed_director__loop_back_tx_bytes_and_feed_pwm_cc_bit, false);
    dma_channel_set_write_addr(DMA_PWM_CC_FEED_DIRECTOR_CHAN, &dma_hw->ch[DMA_PWM_CC_FEED_CHAN].al2_read_addr, false);
    dma_channel_set_trans_count(DMA_PWM_CC_FEED_DIRECTOR_CHAN, 2, false);
}
