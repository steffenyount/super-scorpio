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

#include "tx_pixels_buffer.h"


// 2 buffers x 16 GPIO_TX_PINS x 1 byte = 32
#define TX_ENABLED_ALIGN_SIZE_BITS (5u)
// 2 buffers x 16 GPIO_TX_PINS x 4 bytes = 128
#define TX_PIXELS_ALIGN_SIZE_BITS (7u)
#define PIXELS_PER_FRAME (512u)

volatile bool __scratch_y("super_scorpio") tx_bytes_pending = false;

uint8_t __scratch_y("super_scorpio") __alignment(TX_ENABLED_ALIGN_SIZE_BITS) tx_enabled[2][NUM_GPIO_TX_PINS];
uint8_t __scratch_y("super_scorpio") * gpio_tx_enabled[2] = {
        &tx_enabled[0][0] - GPIO_TX_PINS_BEGIN,
        &tx_enabled[1][0] - GPIO_TX_PINS_BEGIN,
};
uint8_t __scratch_y("super_scorpio") * next_gpio_tx_enabled = &tx_enabled[0][0] - GPIO_TX_PINS_BEGIN;
uint8_t __scratch_y("super_scorpio") * curr_gpio_tx_enabled = &tx_enabled[1][0] - GPIO_TX_PINS_BEGIN;

tx_pixel_t __scratch_y("super_scorpio") __alignment(TX_PIXELS_ALIGN_SIZE_BITS) tx_pixels[2][NUM_GPIO_TX_PINS];
tx_pixel_t __scratch_y("super_scorpio") * gpio_tx_pixels[2] = {
        ((tx_pixel_t *) &tx_pixels[0][0]) - GPIO_TX_PINS_BEGIN,
        ((tx_pixel_t *) &tx_pixels[1][0]) - GPIO_TX_PINS_BEGIN,
};
tx_pixel_t __scratch_y("super_scorpio") * next_gpio_tx_pixels = ((tx_pixel_t *) &tx_pixels[0][0]) - GPIO_TX_PINS_BEGIN;
tx_pixel_t __scratch_y("super_scorpio") * curr_gpio_tx_pixels = ((tx_pixel_t *) &tx_pixels[1][0]) - GPIO_TX_PINS_BEGIN;


const tx_pixel_t tx_pixel_off = {
        .blue = 0u,
        .red = 0u,
        .green = 0u,
        .reserved = 0u,
};

//static uint32_t __scratch_y("super_scorpio") __alignment(5) tx_bytesdump[NUM_PWM_SLICES];

