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

#include "pixel_feeds.h"
#include "pixel_tx/pixel_tx.h"
#include "empty_feed.h"
#include "on_off_feed.h"

spin_lock_t __scratch_y("super_scorpio") * tx_feeds_lock;

//tx_feed_t ** gpio_tx_feeds;
static tx_feed_t __scratch_y("super_scorpio") * tx_feeds[NUM_GPIO_TX_PINS];
tx_feed_t __scratch_y("super_scorpio") ** gpio_tx_feeds = ((tx_feed_t **) &tx_feeds[0]) - GPIO_TX_PINS_BEGIN;

uint16_t __scratch_y("super_scorpio") curr_frame = 0u;
uint16_t __scratch_y("super_scorpio") curr_pixel = 0u;

void __not_in_flash_func(init_tx_feeds)() {
    tx_feeds_lock = spin_lock_init(spin_lock_claim_unused(true));

    spin_lock_unsafe_blocking(tx_feeds_lock);
    for (int gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        if (gpio_num != 16 + SCORPIO_DEFAULT_WS2812_PIN) {
            gpio_tx_feeds[gpio_num] = &empty_feed;

        } else {
            gpio_tx_feeds[gpio_num] = &on_off_feed;
        }
    }

    spin_unlock_unsafe(tx_feeds_lock);
}

