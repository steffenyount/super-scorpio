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

#define GPIO_TX_PINS_BEGIN (8u)
#define GPIO_TX_PINS_END (24u)
#define GPIO_TX_PINS_MASK (0x00ffff00u)
#define NUM_GPIO_TX_PINS (16u)

#define PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE (2u)
#define PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE_MASK (1u << PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE)

#define DMA_TX_BYTES_FEED_CHAN (4u)
#define DMA_TX_BYTES_FEED_CHAN_MASK (1u << DMA_TX_BYTES_FEED_CHAN)
#define DMA_PWM_CC_FEED_CHAN (5u)
#define DMA_PWM_CC_FEED_CHAN_MASK (1u << DMA_PWM_CC_FEED_CHAN)
#define DMA_PWM_CC_FEED_DIRECTOR_CHAN (6u)
#define DMA_PWM_CC_FEED_TRIGGER_CHAN (7u)
#define DMA_PWM_CC_FEED_TRIGGER_CHAN_MASK (1u << DMA_PWM_CC_FEED_TRIGGER_CHAN)


typedef struct {
    uint8_t blue;
    uint8_t red;
    uint8_t green;
    uint8_t reserved;
} tx_pixel_t;

// usage: tx_pixel_t tx_pixel = gpio_tx_pixels[gpio_num]
extern tx_pixel_t * gpio_tx_pixels;

extern uint32_t tx_pixels_enabled;

extern volatile bool tx_bytes_pending;

__force_inline static bool get_tx_pixel_enabled(uint32_t tx_pixels_en, uint gpio_num) {
    return (tx_pixels_en >> gpio_num) & 1u;
}

__force_inline static void set_tx_pixel_enabled(uint gpio_num) {
    tx_pixels_enabled |= (1u << gpio_num);
}

__force_inline static void set_tx_pixel_disabled(uint gpio_num) {
    tx_pixels_enabled &= ~(1u << gpio_num);
}



void init_core0_pixel_tx();

void core1_pixel_tx();

#endif //SUPER_SCORPIO_PIXEL_TX_H