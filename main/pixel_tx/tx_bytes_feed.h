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

#ifndef SUPER_SCORPIO_TX_BYTES_FEED_H
#define SUPER_SCORPIO_TX_BYTES_FEED_H

#include "tick_log/tick_log.h"
#include "pixel_tx.h"

#include "hardware/dma.h"

typedef struct {
    uint8_t tx_enabled;
    uint8_t curr_byte;
} tx_byte_t;

typedef struct {
    tx_byte_t channel_a;
    tx_byte_t channel_b;
} pwm_slice_tx_bytes_t;

// usage: pwm_slice_tx_bytes_t pwm_slice_tx_byte = pwm_slice_tx_bytes[slice_num]
extern pwm_slice_tx_bytes_t pwm_slice_tx_bytes[NUM_PWM_SLICES];
// usage: tx_byte_t tx_byte = gpio_tx_bytes[gpio_num]
extern tx_byte_t * gpio_tx_bytes;

extern volatile bool tx_bytes_feed_done;


void init_dma_tx_bytes_feed();

__force_inline static void trigger_tx_bytes_feed() {
    log_tick("trigger_tx_bytes_feed()");
    // trigger DMA_TX_BYTES_FEED_CHAN
    dma_channel_set_read_addr(DMA_TX_BYTES_FEED_CHAN, &gpio_tx_bytes[GPIO_TX_PINS_BEGIN], true);
}

__force_inline static void wait_for_tx_bytes_feed_done() {
    uint32_t tx_bytes_feed_count = 0u;
    while(!tx_bytes_feed_done) {
        if (tx_bytes_feed_count++ > 1024) break;
//        tight_loop_contents();
    }
    if (tx_bytes_feed_count > 1024) {
        log_tick("tx_bytes_feed_count > 1024");
    }
    tx_bytes_feed_done = false;
}

#endif //SUPER_SCORPIO_TX_BYTES_FEED_H
