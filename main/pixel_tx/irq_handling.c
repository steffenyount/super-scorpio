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

#include "irq_handling.h"
#include "tick_log/tick_log.h"
#include "tx_pixels_buffer.h"
#include "tx_bytes_feed.h"
#include "tx_bytes_to_pwm_cc.pio.h"
#include "pwm_cc_feed.h"
#include "pwm_cc_feed_trigger.h"
#include "pwm_cc_feed_discard.h"
#include "pwm_tx_slices.h"
#include "pixel_feeds/pixel_feeds.h"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"

//static char * __not_in_flash("super_scorpio") pwm_cc_feed_bit_idx_msgs[8] = {
//        "bit - 0",
//        "bit - 1",
//        "bit - 2",
//        "bit - 3",
//        "bit - 4",
//        "bit - 5",
//        "bit - 6",
//        "bit - 7",
//};

static void __not_in_flash_func(on_dma_channel_irq)() {
    static uint8_t __not_in_flash("super_scorpio") pwm_cc_feed_bit_idx = 7;
    const uint32_t dma_hw_ints1 = dma_hw->ints1;

    if (dma_hw_ints1 & DMA_PWM_CC_FEED_CHAN_MASK) { // after last PWM_CC value was fed to pwm
        // Clear the interrupt flag
        dma_hw->ints1 = DMA_PWM_CC_FEED_CHAN_MASK;

//        log_tick("pwm_cc_feed_done");

//        log_tick(pwm_cc_feed_bit_idx_msgs[pwm_cc_feed_bit_idx]);
//
//        uint32_t pwm_slice2_cc_val = pwm_hw->slice[PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE].cc;
//        switch(pwm_slice2_cc_val & 0xffffu) {
//                case 3:
//                    log_tick("fed - 0011");
//                    break;
//                case 7:
//                    log_tick("fed - 0111");
//                    break;
//                case 0:
//                    log_tick("fed - 0000");
//                    break;
//                case 4:
//                    log_tick("fed - 0100");
//                    break;
//                default:
//                    log_tick("fed - ####");
//            }

        // if the pwm_cc_feed_trigger has triggered for the last bit
//        if (!pwm_cc_feed_trigger_is_busy()) {
        if (0 != pwm_cc_feed_bit_idx) {
            pwm_cc_feed_bit_idx--;

        } else {
            pwm_cc_feed_bit_idx = 7;

            if (tx_bytes_pending) {
                trigger_pwm_cc_feeds();

            } else {
                // trigger the irq driven pwm shutdown routine: send WS2812 reset and stops the PWMs
                // enable PWM interrupt
                pwm_clear_irq(PWM_SENTINEL_SLICE);
                hw_set_bits(&pwm_hw->inte, PWM_SENTINEL_SLICE_MASK);

                log_tick("triggering PWM shutdown routine");
            }

            // mark the feed as done
            pwm_cc_feeds_done = true;
            log_tick("pwm_cc_feeds_done");
        }
    }
}

uint32_t __scratch_x("super_scorpio") pwm_count = 0u;
uint16_t __scratch_x("super_scorpio") pwm_slice2_lower_cc_vals[PWM_SLICE_LOWER_CC_SAMPLES] = {0u};

static void __not_in_flash_func(on_pwm_slice_irq)() {
    static volatile bool __scratch_y("super_scorpio") pwm_off_was_sent = false;

    const uint32_t pwm_hw_ints = pwm_hw->ints;
    uint32_t pwm_slice2_cc_val = pwm_hw->slice[PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE].cc;

    if (pwm_hw_ints & PWM_SENTINEL_SLICE_MASK) { // shutdown routine
        // Clear the interrupt flag
        pwm_hw->intr = PWM_SENTINEL_SLICE_MASK;

        if (!pwm_off_was_sent) { // send an off
            log_tick("send a PWM off bit");
            for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
                pwm_hw->slice[slice_num].cc = PWM_CH0_CC_RESET;
                pwm_hw->slice[slice_num].top = WS2812_PWM_RESET_PERIOD - 1u;
            }

            pwm_off_was_sent = true;

        } else { // shut the PWMs down
            log_tick("shut the PWMs down");

            // Prevent further PWM interrupts
            hw_clear_bits(&pwm_hw->inte, PWM_SENTINEL_SLICE_MASK);

            // discard pwm_cc values in pio rxf
            wait_for_rxf_pwm_cc_data_ready();
            trigger_pwm_cc_feed_discard();

            // shutdown and reset pwm
            // stop pwms
            pwm_set_mask_enabled(0u);
            for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
                pwm_hw->slice[slice_num].ctr = PWM_CH0_CTR_RESET;
                pwm_hw->slice[slice_num].top = (WS2812_PWM_BIT_PERIOD - 1u);
            }

            pwm_off_was_sent = false;

            curr_frame++;
            // we're done with frame unlock feeds
            spin_unlock_unsafe(tx_feeds_lock);
        }
    }

    if (pwm_hw_ints & PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE_MASK) { // debug dump
        // Clear the interrupt flag
        pwm_hw->intr = PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE_MASK;
        if (pwm_count < PWM_SLICE_LOWER_CC_SAMPLES) {
            pwm_slice2_lower_cc_vals[pwm_count++] = ((uint16_t)pwm_slice2_cc_val & 0xffffu);
//            pwm_count++;
//            switch(pwm_slice2_cc_val & 0xffffu) {
//                case 0:
//                    log_tick("TICK - 0000");
//                    break;
//                case 3:
//                    log_tick("TICK - 0011");
//                    break;
//                case 7:
//                    log_tick("TICK - 0111");
//                    break;
//                case 4:
//                    log_tick("TICK - 0100");
//                    break;
//                default:
//                    log_tick("TICK - ####");
//            }

        } else {
            // Prevent further interrupts
            pwm_set_irq_enabled(PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE, false);
        }
    }
}

void __not_in_flash_func(init_irq_handling)() {
    // Tell the DMA to raise IRQ line 1 when the channels finish
    // clear and enable the dma interrupts

    dma_channel_acknowledge_irq1(DMA_PWM_CC_FEED_CHAN);
    dma_channel_set_irq1_enabled(DMA_PWM_CC_FEED_CHAN, true);

    // init on_dma_channel_irq irq handler
    irq_set_exclusive_handler(DMA_IRQ_1, on_dma_channel_irq);
    irq_set_enabled(DMA_IRQ_1, true);
    irq_set_priority(DMA_IRQ_1, 0x40); // let the DMA handlers finish when enabling other handlers

//    // init on_pwm_slice_irq irq handler
//    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_slice_irq);
//    irq_set_enabled(PWM_IRQ_WRAP, true);
//    irq_set_priority(PWM_IRQ_WRAP, 0x80);
}

void __not_in_flash_func(init_core0_irq_handling)() {
    systick_hw->csr = 0x5u;

//    // init on_pwm_slice_irq irq handler
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_slice_irq);
    irq_set_enabled(PWM_IRQ_WRAP, true);
//    irq_set_priority(PWM_IRQ_WRAP, 0x40);
}