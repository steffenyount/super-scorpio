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

#include "pwm_feeds.h"
#include "pwm_slices.h"
#include "power_monitor/power_samples.h"
#include "channel_control.h"

#include "hardware/dma.h"
#include "hardware/pwm.h"

#define printbits_n(x,n) for (uint j=x, i=n;i;i--,putchar('0'|((j>>i)&1)))
#define printbits_32(x) printbits_n(x,32)

static bool pwm_cc_data_feed_initialized = false;
static bool pwm_cc_reset_feed_initialized = false;

static uint32_t pwm_cc_off = 0u; // {WS2812_PWM_NO_BIT, WS2812_PWM_NO_BIT}
static uint32_t pwm_cc_A_low = WS2812_PWM_LOW_BIT << PWM_CH0_CC_A_LSB;
static uint32_t pwm_cc_B_low = WS2812_PWM_LOW_BIT << PWM_CH0_CC_B_LSB;
static uint32_t pwm_cc_A_high = WS2812_PWM_HIGH_BIT << PWM_CH0_CC_A_LSB;
static uint32_t pwm_cc_B_high = WS2812_PWM_HIGH_BIT << PWM_CH0_CC_B_LSB;

// Send sufficient reset bits to latch the pixel values (280us), then pause long enough to account for the
// transmission latency (512 pixels * 0.5us = 256us) and the rise/fall times (300us) so that current draw is
// stabilized before completion, and finally allow for a full ADU sample buffer to be collected in this regime
// (#_of_samples * 2us). Note: 8-bits of WS2812 data take 10us to send, and 5 ADU samples take 10us to collect.
static uint32_t bits_reset_count =
        (28u + 26u + 30u + (((POWER_SAMPLES_SIZE * 2u) + 9u) / 10u)) * 8u; /* 1 byte == 10us */

typedef struct {
    uint32_t trans_count;
    uint32_t * read_addr;
} count_and_src_t;

static count_and_src_t counts_and_srcs_for_pwm_cc_feed_director[5];

static inline uint32_t * get_pwm_cc_low_ref_for_gpio(uint8_t gpio_num) {
    return pwm_gpio_to_channel(gpio_num) ? &pwm_cc_B_low : &pwm_cc_A_low;
}

static inline uint32_t * get_pwm_cc_high_ref_for_gpio(uint8_t gpio_num) {
    return pwm_gpio_to_channel(gpio_num) ? &pwm_cc_B_high : &pwm_cc_A_high;
}

static inline void init_pwm_cc_data_feed(uint8_t gpio_num) {
    if (!pwm_cc_data_feed_initialized) {
        dma_channel_claim(DMA_PWM_CC_DATA_FEED_CHAN);
        pwm_cc_data_feed_initialized = true;
    }

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, false);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, pwm_get_dreq(pwm_gpio_to_slice_num(gpio_num)));
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 0u);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, true);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_DATA_FEED_CHAN, &dma_channel_ctrl, false);

//    dma_channel_set_read_addr(DMA_PWM_CC_DATA_FEED_CHAN, &pwm_cc_off, false);
    dma_channel_set_write_addr(DMA_PWM_CC_DATA_FEED_CHAN, &pwm_hw->slice[pwm_gpio_to_slice_num(gpio_num)].cc, false);
//    dma_channel_set_trans_count(DMA_PWM_CC_DATA_FEED_CHAN, 1u, false);
}

static inline void init_pwm_cc_data_feed_director_for_byte_range(uint8_t gpio_num, uint32_t start, uint32_t end) {
    if (!pwm_cc_reset_feed_initialized) {
        dma_channel_claim(DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN);
        pwm_cc_reset_feed_initialized = true;
    }

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 3u); // 2 words x 4 bytes
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN, &dma_channel_ctrl, false);

    // normalize range values
    if (MAX_BYTES_PER_PIXEL_CHANNEL < start) {
        start = MAX_BYTES_PER_PIXEL_CHANNEL;
    }
    if (MAX_BYTES_PER_PIXEL_CHANNEL < end) {
        end = MAX_BYTES_PER_PIXEL_CHANNEL;
    }
    // set up the feed program
    uint16_t idx = 0u;
    if (0u < start) { // how many low bits before start of range?
        counts_and_srcs_for_pwm_cc_feed_director[idx++] = (count_and_src_t) {
            .trans_count = start << 3u,
            .read_addr = get_pwm_cc_low_ref_for_gpio(gpio_num),
        };
    }
    if (start < end) { // how many high bits in range?
        counts_and_srcs_for_pwm_cc_feed_director[idx++] = (count_and_src_t) {
            .trans_count = (end - start) << 3u,
            .read_addr = get_pwm_cc_high_ref_for_gpio(gpio_num),
        };
    }
    if (end < MAX_BYTES_PER_PIXEL_CHANNEL) { // how many low bits after end of range?
        counts_and_srcs_for_pwm_cc_feed_director[idx++] = (count_and_src_t) {
                .trans_count = (MAX_BYTES_PER_PIXEL_CHANNEL - end) << 3u,
                .read_addr = get_pwm_cc_low_ref_for_gpio(gpio_num),
        };
    }
    counts_and_srcs_for_pwm_cc_feed_director[idx++] = (count_and_src_t) {
            .trans_count = bits_reset_count,
            .read_addr = &pwm_cc_off,
    };

    // end of feed sequence
    counts_and_srcs_for_pwm_cc_feed_director[idx++] = (count_and_src_t) {
            .trans_count = 1u,
            .read_addr = NULL,
    };

    dma_channel_set_read_addr(DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN, counts_and_srcs_for_pwm_cc_feed_director, false);
    dma_channel_set_write_addr(DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN, &dma_hw->ch[DMA_PWM_CC_DATA_FEED_CHAN].al3_transfer_count, false);
    dma_channel_set_trans_count(DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN, 2u, false);
}

static void on_dma_pwm_cc_feed_channel_irq() {
    if (dma_hw->ints0 & DMA_PWM_CC_DATA_FEED_CHAN_MASK) {
        // clear the interrupt flag
        dma_hw->ints0 = DMA_PWM_CC_DATA_FEED_CHAN_MASK;

        // shutdown the PWM
        stop_pwm_slice(active_pwm_slice_num);

        // clear irq enabled mask for dma_channel
        hw_clear_bits(&dma_hw->inte0, DMA_PWM_CC_DATA_FEED_CHAN_MASK);
        // remove the on_dma_channel_irq irq handler
        irq_remove_handler(DMA_IRQ_0, on_dma_pwm_cc_feed_channel_irq);
    }
}

static inline void init_pwm_shutdown_handler(uint8_t gpio_num) {
    // init irq mask for dma_channel
    dma_channel_acknowledge_irq0(DMA_PWM_CC_DATA_FEED_CHAN);
    dma_channel_set_irq0_enabled(DMA_PWM_CC_DATA_FEED_CHAN, true);

    // register the on_dma_pwm_cc_feed_channel_irq handler
    irq_add_shared_handler(DMA_IRQ_0, on_dma_pwm_cc_feed_channel_irq, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
}

static inline void trigger_pwm_cc_data_feed_director() {
    dma_hw->multi_channel_trigger = DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN_MASK;
}

void feed_high_bits_to_pwm_slice_for_byte_range(uint8_t gpio_num, uint32_t start, uint32_t end) {
    init_pwm_cc_data_feed_director_for_byte_range(gpio_num, start, end);
    init_pwm_cc_data_feed(gpio_num);
    init_pwm_shutdown_handler(gpio_num);
    trigger_pwm_cc_data_feed_director();
}

