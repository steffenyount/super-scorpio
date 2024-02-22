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

#include "tx_bytes_buffer.h"

pwm_slice_tx_bytes_t __scratch_y("super_scorpio") __alignment(PWM_SLICE_TX_BYTES_ALIGN_SIZE_BITS) pwm_slice_tx_bytes[NUM_PWM_SLICES];
tx_byte_t __scratch_y("super_scorpio") * gpio_tx_bytes = ((tx_byte_t *) &pwm_slice_tx_bytes[0]) - GPIO_TX_PINS_BEGIN;

const uint8_t tx_enabled_on = TX_ENABLED_ON;

const uint8_t tx_enabled_off = TX_ENABLED_OFF;

const tx_byte_t tx_byte_off = {
        .tx_enabled = TX_ENABLED_OFF,
        .curr_byte = TX_ENABLED_OFF,
};

const pwm_slice_tx_bytes_t pwm_slice_tx_bytes_off = {
        .channel_a = tx_byte_off,
        .channel_b = tx_byte_off
};
