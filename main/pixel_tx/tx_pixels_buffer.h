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

#ifndef SUPER_SCORPIO_TX_PIXELS_BUFFER_H
#define SUPER_SCORPIO_TX_PIXELS_BUFFER_H

#include "common.h"
#include "gpio_tx_pins.h"

typedef struct {
    uint8_t blue;
    uint8_t red;
    uint8_t green;
    uint8_t reserved;
} tx_pixel_t;

extern uint8_t tx_enabled[2][NUM_GPIO_TX_PINS];
// usage: uint8_t tx_enabled = gpio_tx_enabled[buffer_num][gpio_num]
extern uint8_t * gpio_tx_enabled[2];
// usage: uint8_t tx_enabled = next_gpio_tx_enabled[gpio_num]
extern uint8_t * next_gpio_tx_enabled;



extern tx_pixel_t tx_pixels[2][NUM_GPIO_TX_PINS];
// usage: tx_pixel_t tx_pixel = gpio_tx_pixels[buffer_num][gpio_num]
extern tx_pixel_t * gpio_tx_pixels[2];
// usage: tx_pixel_t tx_pixel = next_gpio_tx_pixels[gpio_num]
extern tx_pixel_t * next_gpio_tx_pixels;
// usage: tx_pixel_t tx_pixel = curr_gpio_tx_pixels[gpio_num]
extern tx_pixel_t * curr_gpio_tx_pixels;

extern volatile bool tx_bytes_pending;

extern const tx_pixel_t tx_pixel_off;

#endif //SUPER_SCORPIO_TX_PIXELS_BUFFER_H
