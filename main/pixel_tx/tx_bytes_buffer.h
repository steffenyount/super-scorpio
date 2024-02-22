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

#ifndef SUPER_SCORPIO_TX_BYTES_BUFFER_H
#define SUPER_SCORPIO_TX_BYTES_BUFFER_H

#include "common.h"

// 8 PWM_SLICES x 2 PWM_CHANNELS x 2 bytes (curr & enable) = 64
#define PWM_SLICE_TX_BYTES_ALIGN_SIZE_BITS (5u)

#define TX_ENABLED_ON  (0xffu)
#define TX_ENABLED_OFF (0x00u)

typedef struct {
    uint8_t tx_enabled;
    uint8_t curr_byte;
} tx_byte_t;

typedef struct {
    tx_byte_t channel_a;
    tx_byte_t channel_b;
} pwm_slice_tx_bytes_t;

// usage: tx_byte_t tx_byte = gpio_tx_bytes[gpio_num]
extern tx_byte_t * gpio_tx_bytes;
// usage: pwm_slice_tx_bytes_t pwm_slice_tx_byte = pwm_slice_tx_bytes[slice_num]
extern pwm_slice_tx_bytes_t pwm_slice_tx_bytes[NUM_PWM_SLICES];

extern const uint8_t tx_enabled_on;

extern const uint8_t tx_enabled_off;

extern const tx_byte_t tx_byte_off;

extern const pwm_slice_tx_bytes_t pwm_slice_tx_bytes_off;

#endif //SUPER_SCORPIO_TX_BYTES_BUFFER_H
