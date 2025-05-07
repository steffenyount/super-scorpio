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

#include <pico/time.h>
#include "power_samples.h"

#include "hardware/dma.h"
#include "power_monitor_feed.h"

#define SAMPLE_AGGREGATE_ERROR_BIT (0x80000000)
#define NUM_POWER_SAMPLES_FOR_MEDIAN (POWER_SAMPLES_SIZE - 1u)

uint16_t __alignment(POWER_SAMPLES_ALIGN_SIZE_BITS) power_samples[POWER_SAMPLES_SIZE];

static uint16_t __alignment(POWER_SAMPLES_ALIGN_SIZE_BITS) power_samples_snapshot[POWER_SAMPLES_SIZE];

static bool power_samples_snapshotter_initialized = false;

static volatile uint32_t samples_to_aggregate = 0u;
static volatile uint32_t samples_aggregated = 0u;
volatile uint32_t sample_aggregate = 0u;

static inline void init_dma_power_samples_snapshotter() {
    if (!power_samples_snapshotter_initialized) {
        dma_channel_claim(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN);
        power_samples_snapshotter_initialized = true;
    }

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_16);
    channel_config_set_ring(&dma_channel_ctrl, false, POWER_SAMPLES_ALIGN_SIZE_BITS);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &dma_channel_ctrl, false);

    dma_channel_set_read_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &power_samples[1], false);
    dma_channel_set_write_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &power_samples_snapshot, false);
    dma_channel_set_trans_count(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, POWER_SAMPLES_SIZE, false);
}

static inline void trigger_dma_power_samples_snapshotter() {
    dma_channel_set_read_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, incr_power_samples_ptr(next_power_sample_ptr()), false);
    dma_channel_set_write_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &power_samples_snapshot, true);
}

static inline void reset_dma_power_samples_snapshotter() {
    dma_channel_set_read_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &power_samples[1], false);
    dma_channel_set_write_addr(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, &power_samples_snapshot, false);
}

static inline void chain_dma_power_samples_snapshotter() {
    power_monitor_feed_trigger_set_config(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN);
}

static inline void unchain_dma_power_samples_snapshotter() {
    power_monitor_feed_trigger_set_config(DMA_POWER_MONITOR_FEED_TRIGGER_CHAN); // self == disable
}

static inline void update_power_samples_snapshot() {
    init_dma_power_samples_snapshotter();
    trigger_dma_power_samples_snapshotter();
}

static inline void update_power_samples_snapshot_blocking() {
    update_power_samples_snapshot();
    dma_channel_wait_for_finish_blocking(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN);
}

// Algorithm from Numerical recipes in C of 1992
static inline uint16_t quick_select_median(uint16_t values[], uint16_t size) {
    uint16_t low = 0;
    uint16_t high = size - 1;
    uint16_t median = (low + high) >> 1; /* divide by 2 */
    uint16_t middle, ll, hh;

    for (;;) {
        if (high <= low) { /* One element only */
            return (size & 1u) ? values[median] : (values[median] + values[median + 1]) >> 1;
        }
        if (high == low + 1) { /* Two elements only */
            if (values[low] > values[high]) {
                SWAP(values[low], values[high]);
            }
            return (size & 1u) ? values[median] : (values[median] + values[median + 1]) >> 1;
        }
        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) >> 1;
        if (values[middle] > values[high]) {
            SWAP(values[middle], values[high]);
        }
        if (values[low] > values[high]) {
            SWAP(values[low], values[high]);
        }
        if (values[middle] > values[low]) {
            SWAP(values[middle], values[low]);
        }
        /* Swap low item (now in position middle) into position (low+1) */
        SWAP(values[middle], values[low + 1]) ;
        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do ll++; while (values[low] > values[ll]) ;
            do hh--; while (values[hh] > values[low]) ;
            if (hh < ll) {
                break;
            }
            SWAP(values[ll], values[hh]) ;
        }
        /* Swap middle item (in position low) back into correct position */
        SWAP(values[low], values[hh]) ;
        /* Re-set active partition */
        if (hh <= median) {
            low = ll;
        }
        if (hh >= median) {
            high = hh - 1;
        }
    }
}


uint16_t raw_samples[1024][POWER_SAMPLES_SIZE] = {0u};
volatile uint32_t median_samples_count = 0u;
uint16_t median_samples[1024] = {0u};
volatile uint16_t precision_power_sample = 0u;
volatile uint16_t prev_precision_power_sample = 0u;


// My oscilloscope showed the typical base noise within +-3mV on the ADC pin with brief excursions occasionally
// going out to peaks around +-6mV for a duration of less than 2-3us at a time. With the ADC's sample rate at 2us per
// cycle, these excursions may affect one or two sample values per peak. Each ADC LSB should correspond to roughly
// 3.3V/4096 = 0.8mV on the ADC pin, so the +-3mV range of noise spans ~2.9 LSB. By taking the median value of the
// last 15 (~4^1.95) samples we should be able to both eliminate these peak values from our measurements and decrease
// our noise to span about +-0.5 LSB. Next we should be able to further improve our precision by averaging a
// consecutive number N of these median value results, where L = bits of precision to increase, and N = 4^L.

// An 18 pixel LED strip I used for testing showed and increase
// of +72mV/18 = +4mV per LED pixel on the ADC pin. The current sensor of my Super Scorpio FeatherWing board is designed to produce
// +300mV on the ADC pin per Amp of current powering the LED strip (Max 11 Amps = 3.3V), which translates to +60mV on the ADC pin per Watt
// consumed by the 5V LED strips, or ~372 LSBs per Amp and ~74.5 LSBs per Watt consumed by the LED strips.
static inline uint16_t get_median_of_power_samples_snapshot() {
    if (median_samples_count < 1024u) {
        for (uint32_t ii = 0u; ii < POWER_SAMPLES_SIZE - 1u; ii++) {
            raw_samples[median_samples_count][ii] = power_samples_snapshot[ii];
        }
    }

    // remove error'd samples
    uint32_t error_free_sample_count = 0u;
    for (uint32_t idx = 0u; idx < NUM_POWER_SAMPLES_FOR_MEDIAN; idx++) {
        uint16_t raw_power_sample = power_samples_snapshot[idx];
        if (!get_power_sample_error(raw_power_sample)) {
            power_samples_snapshot[error_free_sample_count++] = get_power_sample_value(raw_power_sample);
        }
    }

    uint16_t median_sample;
    // return median of remaining error free samples
    if (0u != error_free_sample_count) {
        median_sample = quick_select_median(power_samples_snapshot, error_free_sample_count);

    } else {
        median_sample = ADC_FIFO_ERR_BITS;
    }

    if (median_samples_count < 1024u) {
        median_samples[median_samples_count++] = median_sample;
    }

    return median_sample;
}

uint16_t get_median_of_power_samples() {
    update_power_samples_snapshot_blocking();
    return get_median_of_power_samples_snapshot();
}

static void on_dma_power_samples_snapshotter_channel_irq() {
    if (dma_hw->ints0 & DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN_MASK) {
        // clear the interrupt flag
        dma_hw->ints0 = DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN_MASK;

        // pause triggering the snapshotter till we're done processing the current snapshot
        unchain_dma_power_samples_snapshotter();

        uint16_t sample = get_median_of_power_samples_snapshot();

        // stop aggregation on ADC error
        if (get_power_sample_error(sample)) {
            sample_aggregate = SAMPLE_AGGREGATE_ERROR_BIT;
            samples_aggregated = samples_to_aggregate;

        } else {
            sample_aggregate += sample;
            samples_aggregated++;
        }

        if (samples_aggregated < samples_to_aggregate) {
            // reset snapshotter values for the next trigger
            reset_dma_power_samples_snapshotter();
            // start triggering the snapshotter again
            chain_dma_power_samples_snapshotter();

        } else { // this was the last pass, shut it down
            // clear irq enabled mask for dma_channel
            hw_clear_bits(&dma_hw->inte0, DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN_MASK);
            // remove the on_dma_channel_irq irq handler
            irq_remove_handler(DMA_IRQ_0, on_dma_power_samples_snapshotter_channel_irq);
        }
    }
}

static inline void init_median_of_power_samples_aggregation_handler() {
    // init irq mask for dma_channel
    dma_channel_acknowledge_irq0(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN);
    dma_channel_set_irq0_enabled(DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN, true);

    // register the on_dma_power_samples_snapshotter_channel_irq handler
    irq_add_shared_handler(DMA_IRQ_0, on_dma_power_samples_snapshotter_channel_irq, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
}

static inline uint32_t get_aggregated_medians_of_power_samples(uint32_t count) {
//    printf("get_aggregated_medians_of_power_samples(%d)\n", count);
    samples_to_aggregate = count;
    samples_aggregated = 0u;
    sample_aggregate = 0u;

    init_dma_power_samples_snapshotter();
    init_median_of_power_samples_aggregation_handler();
    // start triggering the snapshotter
    chain_dma_power_samples_snapshotter();

    uint32_t loops = 0u;
    while(samples_aggregated < samples_to_aggregate) {
        if (loops++ > 300000u) {
            printf("BREAK: loops = %d\n", loops);
            return SAMPLE_AGGREGATE_ERROR_BIT;
        }
        sleep_us(1u);
//        tight_loop_contents();
    }
//    printf("samples_aggregated = %d loops = %d\n", samples_aggregated, loops);


//    printf("get_aggregated_medians_of_power_samples(%d) - done\n", count);

    return sample_aggregate;
}

uint32_t get_precision_power_sample(uint32_t bits) {
//    printf("get_precision_power_sample(%d)\n", bits);
    uint32_t p_sample;
    if (bits < 11u) {
        uint16_t sample = get_median_of_power_samples();
        if (sample == ADC_FIFO_ERR_BITS) {
            p_sample = ADC_FIFO_ERR_BITS;

        } else {
            uint32_t shift_right = 12u - bits;
            // adding half the new LSB for rounding, then shift the new LSB to position 0
            p_sample = (sample + (1u << (shift_right - 1))) >> shift_right;
        }

    } else {
        uint32_t new_bits_precision = bits - 10u;
        uint32_t sample_count = 1 << (new_bits_precision << 1); /* sample_count = 4^new_bits_precision */
        uint32_t aggregate = get_aggregated_medians_of_power_samples(sample_count);
        if (aggregate == SAMPLE_AGGREGATE_ERROR_BIT) {
            p_sample = ADC_FIFO_ERR_BITS;

        } else {
            // aggregate value will be (12 + (new_bits_precision * 2)) bits wide, so the target LSB will be at
            // position (12 + (new_bits_precision * 2) - bits).
            uint32_t shift_right = 12u + (new_bits_precision << 1) - bits;
            // adding half the new LSB for rounding, then shift the new LSB to position 0, for avg of aggregated samples
            p_sample = (aggregate + (1u << (shift_right - 1))) >> shift_right;
        }
    }
    precision_power_sample = p_sample;
//    printf("get_precision_power_sample(%d) -  done\n", bits);

    return p_sample;
}
