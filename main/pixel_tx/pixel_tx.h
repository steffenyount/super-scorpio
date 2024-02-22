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

#ifndef SUPER_SCORPIO_PIXEL_TX_H
#define SUPER_SCORPIO_PIXEL_TX_H

#include "common.h"

#define PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE (2u)
#define PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE_MASK (1u << PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE)

#define DMA_TX_BYTES_FEED_CHAN (4u)
#define DMA_TX_BYTES_FEED_CHAN_MASK (1u << DMA_TX_BYTES_FEED_CHAN)
#define DMA_TX_BYTES_FEED_DIRECTOR_CHAN (5u)
#define DMA_TX_BYTES_FEED_DIRECTOR_CHAN_MASK (1u << DMA_TX_BYTES_FEED_DIRECTOR_CHAN)
#define DMA_PWM_SLICE_TX_BYTES_FEED_CHAN (6u)
#define DMA_PWM_SLICE_TX_BYTES_FEED_CHAN_MASK (1u << DMA_PWM_SLICE_TX_BYTES_FEED_CHAN)
#define DMA_PWM_CC_FEED_CHAN (7u)
#define DMA_PWM_CC_FEED_CHAN_MASK (1u << DMA_PWM_CC_FEED_CHAN)
#define DMA_PWM_CC_FEED_DIRECTOR_CHAN (8u)
#define DMA_PWM_CC_FEED_TRIGGER_CHAN (9u)
#define DMA_PWM_CC_FEED_TRIGGER_CHAN_MASK (1u << DMA_PWM_CC_FEED_TRIGGER_CHAN)
#define DMA_PWM_CC_FEED_DISCARD_CHAN (10u)
#define DMA_PWM_CC_FEED_DISCARD_CHAN_MASK (1u << DMA_PWM_CC_FEED_DISCARD_CHAN)


extern uint32_t tx_pixels_enabled;

static inline bool get_tx_pixel_enabled(uint32_t tx_pixels_en, uint gpio_num) {
    return (tx_pixels_en >> gpio_num) & 1u;
}

static inline void set_tx_pixel_enabled(uint gpio_num) {
    tx_pixels_enabled |= (1u << gpio_num);
}

static inline void set_tx_pixel_disabled(uint gpio_num) {
    tx_pixels_enabled &= ~(1u << gpio_num);
}



void init_core0_pixel_tx();

void core1_pixel_tx();

#endif //SUPER_SCORPIO_PIXEL_TX_H