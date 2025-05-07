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

#ifndef SUPER_SCORPIO_ON_OFF_FEED_H
#define SUPER_SCORPIO_ON_OFF_FEED_H

#include "pixel_feed_base.h"

typedef struct on_off_feed {
    pixel_feed_base_t;
    rgbw_pixel_t src_pixel;
} on_off_feed_t;

void set_on_off_feed_chain(uint8_t count, const uint8_t tx_gpio_nums[count], uint16_t chain_offset);

static inline void set_on_off_feed_with_offset(uint8_t gpio_num, uint16_t start_offset) {
    set_on_off_feed_chain(1, (uint8_t [1]) {gpio_num}, start_offset);
}

static inline void set_on_off_feed(uint8_t gpio_num) {
    set_on_off_feed_chain(1, (uint8_t [1]) {gpio_num}, 0u);
}

#endif //SUPER_SCORPIO_ON_OFF_FEED_H
