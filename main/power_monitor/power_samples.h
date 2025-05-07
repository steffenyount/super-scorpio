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

#ifndef SUPER_SCORPIO_POWER_SAMPLES_H
#define SUPER_SCORPIO_POWER_SAMPLES_H

#include "power_monitor.h"

#include <stdio.h>
#include "hardware/regs/adc.h"
#include "hardware/structs/dma.h"


#define POWER_SAMPLES_SIZE (16u)
// 2 bytes x 16 measurements = 32
#define POWER_SAMPLES_ALIGN_SIZE_BITS (5u)

extern uint16_t power_samples[POWER_SAMPLES_SIZE];

extern volatile uint32_t median_samples_count;
extern uint16_t raw_samples[1024][POWER_SAMPLES_SIZE];
extern uint16_t median_samples[1024];
extern volatile uint16_t prev_precision_power_sample;

extern volatile uint32_t sample_aggregate;
extern volatile uint16_t precision_power_sample;


static inline uint32_t next_power_sample_index() {
    uint32_t next_power_sample_idx = (POWER_SAMPLES_SIZE - dma_hw->ch[DMA_POWER_MONITOR_FEED_CHAN].transfer_count);
    if (POWER_SAMPLES_SIZE <= next_power_sample_idx) {
        next_power_sample_idx -= POWER_SAMPLES_SIZE;
    }
    return next_power_sample_idx;
}

static inline uint16_t * next_power_sample_ptr() {
    // Workaround for errata: RP2040-E12
    // We can't just return *dma_hw->ch[DMA_POWER_MONITOR_FEED_CHAN].write_addr
    return &power_samples[next_power_sample_index()];
}

static inline uint16_t * incr_power_samples_ptr(uint16_t * curr_ptr) {
    return (curr_ptr == &power_samples[POWER_SAMPLES_SIZE - 1u]) ? curr_ptr - (POWER_SAMPLES_SIZE - 1u) : curr_ptr + 1;
}

static inline uint16_t * decr_power_samples_ptr(uint16_t * curr_ptr) {
    return (curr_ptr == &power_samples[0u]) ? curr_ptr + (POWER_SAMPLES_SIZE - 1u) : curr_ptr - 1;
}


static inline uint16_t get_power_sample_error(uint16_t raw_power_sample) {
    return raw_power_sample & ADC_FIFO_ERR_BITS;
}

static inline uint16_t get_power_sample_value(uint16_t raw_power_sample) {
    return raw_power_sample & ADC_FIFO_VAL_BITS;
}

static inline uint16_t get_corrected_power_sample_value(uint16_t raw_power_sample) {
    raw_power_sample &= ADC_FIFO_VAL_BITS;
    // TODO: calibrate for rp2040's INL and DNL
    switch (raw_power_sample & 0xe00u) {
        case 0u:
            return raw_power_sample;
        case 512u:
        case 1024u:
            return raw_power_sample;
        case 1536u:
        case 2048u:
            return raw_power_sample;
        case 2560u:
        case 3072u:
            return raw_power_sample;
        case 3584u:
            return raw_power_sample;
        default:
            return raw_power_sample;
    }
}

uint16_t get_median_of_power_samples();

uint32_t get_precision_power_sample(uint32_t bits);


#endif //SUPER_SCORPIO_POWER_SAMPLES_H
