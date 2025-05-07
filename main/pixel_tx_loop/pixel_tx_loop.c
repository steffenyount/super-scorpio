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

#include "pixel_tx_loop.h"
#include "tick_log/tick_log.h"
#include "tx_bytes_to_gpio_pins.pio.h"
#include "tx_bytes_feed.h"
#include "tx_data_feeds.h"

#include <stdio.h>
#include "hardware/pio.h"
#include "hardware/sync.h"

#define printbits_n(x,n) for (uint j=x, i=n;i;i--,putchar('0'|((j>>i)&1)))
#define printbits_32(x) printbits_n(x,32)
#define printbits_16(x) printbits_n(x,16)


__force_inline static uint32_t get_sm0_tx_enabled_bits(uint32_t tx_pixels_en) {
    return (tx_pixels_en >> (GPIO_TX_PINS_BEGIN + (0u * NUM_PIO_STATE_MACHINE_TX_PINS))) & PIO_STATE_MACHINE_TX_PINS_MASK;
}

__force_inline static uint32_t get_sm1_tx_enabled_bits(uint32_t tx_pixels_en) {
    return (tx_pixels_en >> (GPIO_TX_PINS_BEGIN + (1u * NUM_PIO_STATE_MACHINE_TX_PINS))) & PIO_STATE_MACHINE_TX_PINS_MASK;
}

__force_inline static uint32_t get_sm2_tx_enabled_bits(uint32_t tx_pixels_en) {
    return (tx_pixels_en >> (GPIO_TX_PINS_BEGIN + (2u * NUM_PIO_STATE_MACHINE_TX_PINS))) & PIO_STATE_MACHINE_TX_PINS_MASK;
}

__force_inline static uint32_t get_sm3_tx_enabled_bits(uint32_t tx_pixels_en) {
    return (tx_pixels_en >> (GPIO_TX_PINS_BEGIN + (3u * NUM_PIO_STATE_MACHINE_TX_PINS))) & PIO_STATE_MACHINE_TX_PINS_MASK;
}

__force_inline static void stage_tx_enabled_bits() {
    const uint32_t tx_pixels_en = get_tx_pixels_enabled();
    tx_data.sm_enabled_bits[0u] = get_sm0_tx_enabled_bits(tx_pixels_en);
    tx_data.sm_enabled_bits[1u] = get_sm1_tx_enabled_bits(tx_pixels_en);
    tx_data.sm_enabled_bits[2u] = get_sm2_tx_enabled_bits(tx_pixels_en);
    tx_data.sm_enabled_bits[3u] = get_sm3_tx_enabled_bits(tx_pixels_en);
}

__force_inline static void wait_for_pio_txf_level_blocking() {
    // assuming all pio state machines are running at the same rate, we'll use the SM3's txf level as our sentinel
    while ((pio1->flevel & PIO_FLEVEL_TX3_BITS) > (6u << PIO_FLEVEL_TX3_LSB)) { // wait for 2 of 8 slots available
        tight_loop_contents();
    }
}

static uint32_t prev_tx_loop_end_systicks[2] = {0u};

__force_inline static void capture_tx_loop_end_systick_even() {
    prev_tx_loop_end_systicks[0] = systick_hw->cvr;
}

__force_inline static void capture_tx_loop_end_systick_odd() {
    prev_tx_loop_end_systicks[1] = systick_hw->cvr;
}

__force_inline static void check_pio_stalled_prev_tx_loop_even() {
    if (get_prev_pio1_txstall_bits()) {
        core1_log_tick_with_value("**** PIO stalled **** (prev tx_loop took %d ticks)",
                                  prev_tx_loop_end_systicks[1] - prev_tx_loop_end_systicks[0]);
    }
}

__force_inline static void check_pio_stalled_prev_tx_loop_odd() {
    if (get_prev_pio1_txstall_bits()) {
        core1_log_tick_with_value("**** PIO stalled **** (prev tx_loop took %d ticks)",
                                  prev_tx_loop_end_systicks[0] - prev_tx_loop_end_systicks[1]);
    }
}

void run_pixel_tx_loop() {
    printf("run_pixel_tx_loop()\n");

    init_tx_byte_feeds();
    init_tx_data_feeds();
    init_pio1_for_tx_bytes_to_gpio_pins_programs();

    // init the prev_tx_loop_end_systick value as if there were a 0th iteration
    capture_tx_loop_end_systick_even();

    // on the 1st iteration the pio will have been stalled and prev_pio1_fdebug will not contain captured values
    // then, on the 2nd iteration the prev_pio1_fdebug flags will have captured the 1st iteration's stalled state
    // so, we'll skip the prev_pio1_fdebug based stall checks here for both of these two iterations

    // 1st (odd) iteration
    advance_tx_bytes();
    wait_for_prev_tx_data_fed_blocking();
    stage_tx_enabled_bits();
//    if (get_tx_pixels_enabled()) {
//    core1_log_tick_with_value("ready to feed after %4d ticks",
//                              prev_tx_loop_end_systicks[0] - systick_hw->cvr);
//    }
    wait_for_pio_txf_level_blocking();
    trigger_next_tx_data_feed();
    // no check for pio stalled in previous tx_loop
    capture_tx_loop_end_systick_odd();

    // 2nd (even) iteration
    advance_tx_bytes();
    wait_for_prev_tx_data_fed_blocking();
    stage_tx_enabled_bits();
//    if (get_tx_pixels_enabled()) {
//    core1_log_tick_with_value("ready to feed after %4d ticks",
//                              prev_tx_loop_end_systicks[1] - systick_hw->cvr);
//    }
    wait_for_pio_txf_level_blocking();
    trigger_next_tx_data_feed();
    // no check for pio stalled in previous tx_loop
    capture_tx_loop_end_systick_even();

    while (true) {
        // odd iteration
        advance_tx_bytes();
        wait_for_prev_tx_data_fed_blocking();
        stage_tx_enabled_bits();
//    if (get_tx_pixels_enabled()) {
//        core1_log_tick_with_value("ready to feed after %4d ticks",
//                                  prev_tx_loop_end_systicks[0] - systick_hw->cvr);
//    }
        wait_for_pio_txf_level_blocking();
        trigger_next_tx_data_feed();
        check_pio_stalled_prev_tx_loop_even();
        capture_tx_loop_end_systick_odd();

        // even iteration
        advance_tx_bytes();
        wait_for_prev_tx_data_fed_blocking();
        stage_tx_enabled_bits();
//    if (get_tx_pixels_enabled()) {
//        core1_log_tick_with_value("ready to feed after %4d ticks",
//                                  prev_tx_loop_end_systicks[1] - systick_hw->cvr);
//    }
        wait_for_pio_txf_level_blocking();
        trigger_next_tx_data_feed();
        check_pio_stalled_prev_tx_loop_odd();
        capture_tx_loop_end_systick_even();
    }
}

