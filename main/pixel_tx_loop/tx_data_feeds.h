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

#ifndef SUPER_SCORPIO_TX_DATA_FEEDS_H
#define SUPER_SCORPIO_TX_DATA_FEEDS_H

#include "common.h"
#include "tick_log/tick_log.h"

#include "hardware/regs/pio.h"

// 4 bytes x 4 sm_enabled_bits + 4 bytes x 4 sm_bytes = 32
#define TX_DATA_ALIGN_SIZE_BITS (5u)

typedef struct {
    uint32_t sm_enabled_bits[NUM_PIO_STATE_MACHINES];
    union {
        uint8_t bytes[NUM_TX_PINS];
        uint32_t sm_bytes[NUM_PIO_STATE_MACHINES];
    };
} tx_data_t;

extern tx_data_t tx_data;


// usage: gpio_tx_bytes[gpio_num]
extern uint8_t * const gpio_tx_bytes;

extern volatile uint32_t tx_data_pending_index;
extern volatile uint32_t tx_data_fed_index;

extern volatile uint32_t prev_pio1_fdebug;

void init_tx_data_feeds();

void trigger_next_tx_data_feed();

static inline void wait_for_prev_tx_data_fed_blocking() {
//    uint32_t loops = 0u;
    while (tx_data_fed_index != tx_data_pending_index) {
//        if (loops++ > 1000u) break;
//        sleep_us(1u);
        tight_loop_contents();
    }
//    if (loops > 0u) {
//        core1_log_tick_with_value("wait_for_prev_tx_data_fed_blocking() waited with loops == %d", loops);
//        if (loops > 1000u) {
//            core1_log_tick_with_value("wait_for_prev_tx_data_fed_blocking() broke with loops == %d", loops);
//        }
//    }
}

__force_inline static uint32_t get_prev_pio1_txstall_bits() {
    return (prev_pio1_fdebug & PIO_FDEBUG_TXSTALL_BITS);
}

#endif //SUPER_SCORPIO_TX_DATA_FEEDS_H
