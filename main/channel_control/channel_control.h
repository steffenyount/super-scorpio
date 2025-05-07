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

#ifndef SUPER_SCORPIO_CHANNEL_CONTROL_H
#define SUPER_SCORPIO_CHANNEL_CONTROL_H

#include "common.h"

#define MAX_BYTES_PER_PIXEL_CHANNEL (512u * 3u)

void set_gpio_channel_pixels_on_for_byte_range(uint8_t gpio_num, uint32_t start, uint32_t end);

static inline void set_gpio_channel_pixels_off(uint8_t gpio_num) {
    set_gpio_channel_pixels_on_for_byte_range(gpio_num, MAX_BYTES_PER_PIXEL_CHANNEL, MAX_BYTES_PER_PIXEL_CHANNEL);
}

static inline void set_gpio_channel_pixels_on(uint8_t gpio_num) {
    set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, MAX_BYTES_PER_PIXEL_CHANNEL);
}

static inline void set_all_gpio_channel_pixels_off() {
    for (uint8_t gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        set_gpio_channel_pixels_off(gpio_num);
    }
}

static inline void set_all_gpio_channel_pixels_on() {
    for (uint8_t gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        set_gpio_channel_pixels_on(gpio_num);
    }
}

void init_pixel_control();

#endif //SUPER_SCORPIO_CHANNEL_CONTROL_H
