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

#ifndef SUPER_SCORPIO_PIXEL_FEEDS_H
#define SUPER_SCORPIO_PIXEL_FEEDS_H

#include "hardware/sync.h"

typedef struct tx_feed_t {
    void (*open_frame)(struct tx_feed_t * this, uint8_t gpio_num);
    bool (*advance_pixel)(struct tx_feed_t * this, uint8_t gpio_num);
    void (*close_frame)(struct tx_feed_t * this, uint8_t gpio_num);
} tx_feed_t;

extern spin_lock_t * tx_feeds_lock;

// usage: gpio_tx_feeds[gpio_num]->advance_pixel(gpio_tx_feeds[gpio_num])
extern tx_feed_t ** gpio_tx_feeds;

extern uint16_t curr_frame;
extern uint16_t curr_pixel;

void init_tx_feeds();


#endif //SUPER_SCORPIO_PIXEL_FEEDS_H
